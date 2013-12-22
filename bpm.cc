#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <list>
#include <tuple>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sqlite3.h>

#define BAUDRATE        B4800
#define MODEMDEVICE     "/dev/ttyUSB0"
#define _POSIX_SOURCE   1

/**
 * TODO:
 *  * Specify serial port as parameter.
 *  * Set database dir as parameter.
 *  * Support config file?
 *  * Range support.
 *  --* Average support.--
 */

namespace BPM
{

    /**
     * The measurement field as returned from the BPM.
     * This struct is 'raw' read from the serial link.
     * Code should always uses accessors, this 'corrects' for
     * the values in the protocol.
     */
    typedef struct  {
private:
        uint8_t mag;
        uint8_t systolic;
        uint8_t diastolic; 
        uint8_t bpm;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t min;
        uint8_t year; 
public:
        void set_systolic(uint8_t val ) { this->systolic = val-25;}
        void set_diastolic(uint8_t val ) { this->diastolic= val-25;}
        void set_bpm(uint8_t val ) { this->bpm = val;}
        uint8_t get_systolic() { return this->systolic+25; }
        uint8_t get_diastolic() { return this->diastolic+25; }
        uint8_t get_bpm() { return this->bpm; }
        
        time_t get_time()
        {
            struct tm  timestr;
            timestr.tm_sec = 0;
            timestr.tm_min = this->min;
            timestr.tm_hour = this->hour;
            timestr.tm_mon = this->month-1;
            timestr.tm_mday = this->day;
            timestr.tm_year = this->year+100;
            return mktime(&timestr);;
        }
        void set_time(time_t time)
        {
            struct tm *timestr;
            timestr = localtime(&time);
            this->min = timestr->tm_min;
            this->hour = timestr->tm_hour;
            this->day = timestr->tm_mday;
            this->month = timestr->tm_mon+1;
            this->year = timestr->tm_year-100;
        }
        // Dump content of struct.
        void print()
        {
            char buffer[1024];
            struct tm *timestr;
            time_t t = this->get_time();
            timestr = localtime(&t);
            strftime(buffer, 1024, "%c", timestr);
            printf("%s %d %d\n", buffer, this->get_systolic(), this->get_diastolic());
        }
        void print_csv()
        {
            printf("\"%llu\",\"%d\",\"%d\",\"%d\"\n",
                (long long unsigned)this->get_time(),
                this->get_systolic(),
                this->get_diastolic(),
                this->get_bpm());
        }
    }measurement;

    /**
     * Storage
     *
     * Store fields in an sqlite3 database.
     */
    class Storage
    {
        private:
            sqlite3 *handle = nullptr;
            sqlite3_stmt *insert_stmt = nullptr;
            sqlite3_stmt *list_all_stmt = nullptr;
            sqlite3_stmt *get_average_stmt = nullptr;

            /**
             * Add the database definition.
             */
            void check_database()
            {
                const char *const statement =  "CREATE TABLE IF NOT EXISTS bpp ("
                        "time INTEGER UNIQUE PRIMARY KEY,"
                        "systolic INTEGER,"
                        "diastolic INTEGER,"
                        "bpm INTEGER);";

                char *errmsg = nullptr;
                int res = sqlite3_exec(this->handle, statement, nullptr, nullptr, &errmsg);
                if( res != SQLITE_OK ) {
                    fprintf(stderr, "Failed to create table: %s\n", errmsg);
                }

            }

            /**
             * Precompile SQL statements.
             */
            void prepare_statements()
            {
                const char * const insert_db_str = "INSERT OR IGNORE INTO bpp VALUES(?,?,?,?);";
                int retv = sqlite3_prepare_v2(this->handle,
                        insert_db_str, -1,
                        &(this->insert_stmt),nullptr);
                if( retv != SQLITE_OK ) {
                    fprintf(stderr, "Failed to prepare statement: %s:%i\n", insert_db_str,retv);
                }

                const char * const list_all_str = "SELECT * from bpp ORDER BY time;";
                retv = sqlite3_prepare_v2(this->handle,
                        list_all_str, -1,
                        &(this->list_all_stmt),nullptr);
                if( retv != SQLITE_OK ) {
                    fprintf(stderr, "Failed to prepare statement: %s:%i\n", list_all_str,retv);
                }

                const char * const get_average_str = "SELECT avg(systolic), avg(diastolic) FROM bpp;";
                retv = sqlite3_prepare_v2(this->handle,
                        get_average_str, -1,
                        &(this->get_average_stmt), nullptr);
                if( retv != SQLITE_OK ) {
                    fprintf(stderr, "Failed to prepare statement: %s:%i\n", get_average_str,retv);
                }
            }

        public:
            void add(measurement &msg)
            {
                    sqlite3_bind_int64(this->insert_stmt, 1, msg.get_time());
                    sqlite3_bind_int(this->insert_stmt, 2, msg.get_systolic());
                    sqlite3_bind_int(this->insert_stmt, 3, msg.get_diastolic());
                    sqlite3_bind_int(this->insert_stmt, 4, msg.get_bpm());
                    sqlite3_step(this->insert_stmt);

                    sqlite3_reset(this->insert_stmt);
            }

            std::tuple<uint8_t, uint8_t> average()
            {
                std::tuple<uint8_t, uint8_t> retv(0,0);

                int rc = sqlite3_step(this->get_average_stmt);
                switch(rc)  {
                    case SQLITE_DONE:
                        break;
                    case SQLITE_ROW:
                        {
                            std::get<0>(retv) = sqlite3_column_int(this->get_average_stmt,0);
                            std::get<1>(retv) = sqlite3_column_int(this->get_average_stmt,1);
                        }
                        break;
                    default:
                        fprintf(stderr, "Error iterating table: %s\n",
                                sqlite3_errmsg(this->handle)); 
                        break;    
                }

                // Reset the query.
                sqlite3_reset(this->list_all_stmt);
                return retv;
            }

            std::list<measurement>  list()
            {
                std::list<measurement> list;
                int rc;
                do{                
                    rc = sqlite3_step(this->list_all_stmt);
                    switch(rc)  {
                        case SQLITE_DONE:
                            break;
                        case SQLITE_ROW:
                            {
                                measurement msg;
                                time_t time = sqlite3_column_int64(this->list_all_stmt,0);
                                msg.set_time(time);
                                msg.set_systolic(sqlite3_column_int(this->list_all_stmt,1));
                                msg.set_diastolic(sqlite3_column_int(this->list_all_stmt,2)); 
                                msg.set_bpm(sqlite3_column_int(this->list_all_stmt,3));
                                list.push_back(msg);
                            }
                            break;
                        default:
                            fprintf(stderr, "Error iterating table: %s\n",
                                    sqlite3_errmsg(this->handle)); 
                            break;    
                    }

                }while(rc == SQLITE_ROW);
                list.reverse();
                // Reset the query.
                sqlite3_reset(this->list_all_stmt);
                return list;
            }


            Storage()
            {
                char *path = nullptr;
                // Create path.
                const char * filename = ".bpm.sqlite3";
                const char * homedir = getenv("HOME");
                ssize_t size = strlen(filename)+strlen(homedir)+2;
                path = (char *) new char[size];
                snprintf(path, size, "%s/%s", homedir, filename); 
                printf("Opening db: %s\n", path);


                int res = sqlite3_open(path, &this->handle);
                if ( res != SQLITE_OK ) {
                    fprintf(stderr, "Failed to open database: %s\n", path);
                    delete path;
                    exit(1);
                    return;
                }

                check_database();
                prepare_statements();

            }

            ~Storage()
            {
                printf("Closing database\n");
                if( this->insert_stmt != nullptr ) {
                    sqlite3_finalize(this->insert_stmt);
                    this->insert_stmt = nullptr;
                }
                if( this->list_all_stmt != nullptr ) {
                    sqlite3_finalize(this->list_all_stmt);
                    this->list_all_stmt = nullptr;
                }
                if( this->get_average_stmt != nullptr ) {
                    sqlite3_finalize(this->get_average_stmt);
                    this->get_average_stmt = nullptr;
                }
                if(this->handle != nullptr) {
                    int res = sqlite3_close(this->handle);
                    if(res != SQLITE_OK) {

                    }
                }
            }

    };

    class BM58 
    {
        private:
            // File handle
            int fd = 0;
            struct termios oldtio;

            void Read(int fd,uint8_t *buffer,ssize_t size)
            {
                ssize_t d = 0;
                while(d < size) {
                    d += read(fd, &buffer[d], 1);
                }
            }

            bool ping()
            {
                uint8_t msg = 0xAA;
                write(fd, &msg, 1); 

                read(fd, &msg, 1);

                return (msg == 0x55);
            }
            void print_name()
            {
                uint8_t resp[33];
                uint8_t msg = 0xA4;

                write(fd, &msg, 1);

                Read(fd,resp,32);             

                resp[32] = '\0';
                printf("Found device: %s\n", resp);
            }

         public:
            int get_num_measurements()
            {
                uint8_t msg = 0xA2;
                write(fd, &msg, 1); 
                read(fd, &msg, 1);
                return msg;
            }


            measurement get_measurement(int id)
            {
                measurement retv;
                uint8_t msg = 0xA3;
                write(fd, &msg, 1);
                msg = id+1;
                write(fd, &msg, 1);

                Read(fd, (uint8_t *)&retv, sizeof(measurement)); 
                
                return retv;
            }
            bool open_device()
            {
                struct termios newtio;


                fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY ); 
                if (fd <0) {
                    perror(MODEMDEVICE);
                    return false;
                }

                /* save current serial port settings */
                tcgetattr(fd,&oldtio);

                /* clear struct for new port settings */
                memset(&newtio, 0,sizeof(newtio)); 
                /* configure port */
                newtio.c_cflag = BAUDRATE | CS8;
                newtio.c_iflag = 0; 
                newtio.c_oflag = 0;
                newtio.c_lflag = 0;       //ICANON;
                newtio.c_cc[VMIN]=1;
                newtio.c_cc[VTIME]=0;         

                /* set settings. */
                tcflush(fd, TCIFLUSH);
                tcsetattr(fd,TCSANOW,&newtio);

                if(!ping()) {
                    fprintf(stderr, "Failed to ping BPM58\n");
                    return false;
                }
                print_name();
                return true;
            }
            void close_device()
            {
                if( fd > 0 ) {
                    printf("Closing link to BPM\n");
                    /* restore the old port settings */
                    tcsetattr(fd,TCSANOW,&oldtio);
                    close(fd);
                    fd = 0;
                }
            }
        public:
            BM58()
            {
            }
            ~BM58()
            {
                close_device();
            }
    };


    class Main {
        private:
                Storage storage;
                BM58    bm58;
        void import()
        {
            if(bm58.open_device()) {
                int num_records = bm58.get_num_measurements();
                printf("Found: %03d records\n", num_records);
                for ( int i = 0; i < num_records; i++) {
                    auto msg = bm58.get_measurement(i);
                    fputs("\033[A\033[2K",stdout);
                    printf("%03d/%03d\n", i+1, num_records); 
                    storage.add(msg);
                }
            }
        }
        /**
         * 
         */
        void help()
        {

        }

        /**
         * list measurements
         */
        void list()
        {
            printf("Listing nodes\n");
            auto list = storage.list();
            for ( auto it = list.begin(); it != list.end(); it++) {
                (*it).print();
            }
        }
        void list_csv()
        {
            auto list = storage.list();
            for ( auto it = list.begin(); it != list.end(); it++) {
                (*it).print_csv();
            }
        }
        void print_avg()
        {
            auto val = storage.average();
            printf("Average:\n\tSystolic: %d\n\tDiastolic: %d\n",
                std::get<0>(val), std::get<1>(val));
        }

        public:
            Main() {}

            int run (int argc, char *argv[])
            {
                if(argc > 1) {
                    if( strncmp(argv[1], "import", 6) == 0) {
                        this->import();
                    } else if ( strncmp(argv[1], "list", 4) == 0) {
                        this->list();
                    } else if ( strncmp(argv[1], "csv", 3) == 0) {
                        this->list_csv();
                    } else if  ( strncmp (argv[1], "avg", 2) == 0) {
                        this->print_avg();
                    } else {
                        this->help();
                    }
                }
                else
                {
                    this->help();
                }

                return EXIT_SUCCESS;
            }

    };
}

int main(int Parm_Count, char *Parms[])
{
    auto m = BPM::Main();

    return m.run(Parm_Count, Parms);
}
