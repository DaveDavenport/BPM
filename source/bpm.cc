/*
 *    This file is part of BPM.
 *    Written by Qball Cow <qball@gmpclient.org> 2013
 *
 *    BPM is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License.
 *
 *    BPM is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with BPM.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdint.h>
#include <time.h>
#include <list>
#include <tuple>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sqlite3.h>

/**
 * Serial port speed.
 * This is 4800bits/sec.
 */
#define BAUDRATE        B4800

/**
 * Default serial port device
 */
#define MODEMDEVICE     "/dev/ttyUSB0"


/**
 * @todo * Specify serial port as parameter.--(implemented using env)
 *
 * @todo * Set database dir as parameter.-- (implemented using env)
 *
 * @todo * Support config file?
 *
 * @todo * Range support.
 */

namespace BPM
{

    enum WHO_class {
        GRADE_3,
        GRADE_2,
        GRADE_1,
        HIGH,
        NORMAL,
        OPTIMAL,
        N_CLASS
    };
    const char *WHO_class_str[N_CLASS] = {
        "Severe Hypertension",
        "Moderate hypertension",
        "Mild hypertension",
        "High-normal",
        "Normal",
        "Optimal"
    };
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


            WHO_class get_classification ( ) {
                auto wc = OPTIMAL;

                if ( systolic >= ( 180-25 ) ) {
                    wc = GRADE_3;
                } else if ( systolic >= ( 160 -25 ) )  {
                    wc = GRADE_2;
                } else if ( systolic >= ( 140 -25 ) )  {
                    wc = GRADE_1;
                } else if ( systolic >= ( 130 -25 ) )  {
                    wc = HIGH;
                } else if ( systolic >= ( 120 -25 ) )  {
                    wc = NORMAL;
                }

                if ( diastolic >= ( 110-25 ) ) {
                    wc = GRADE_3;
                } else if ( diastolic >= ( 100 -25 ) ) {
                    wc = GRADE_2;
                } else if ( diastolic >= ( 90-25 ) ) {
                    wc = GRADE_1;
                } else if ( diastolic >= ( 85-25 ) ) {
                    wc = HIGH;
                } else if ( diastolic >= ( 80-25 ) ) {
                    wc = NORMAL;
                }

                return wc;
            }

            /**
             * @param val the new systolic value.
             * Set the systolic value for this measurement
             */
            void set_systolic( uint8_t val ) {
                this->systolic = val-25;
            }
            /**
             * Get the systolic value
             *
             * @returns the systolic value.
             */
            uint8_t get_systolic() {
                return this->systolic+25;
            }
            /**
             * @param val the new diastolic value.
             * Set the diastolic value for this measurement
             */
            void set_diastolic( uint8_t val ) {
                this->diastolic= val-25;
            }
            /**
             * Get the diastolic value
             * @returns the diastolic value.
             */
            uint8_t get_diastolic() {
                return this->diastolic+25;
            }
            /**
             * @param val the new bpm value.
             * Set the bpm (beats per minute) value for this measurement
             */
            void set_bpm( uint8_t val ) {
                this->bpm = val;
            }
            /**
             * Get the bpm (beats per minute)
             * @returns the BPM.
             */
            uint8_t get_bpm() {
                return this->bpm;
            }

            /**
             * Accessors
             */

            /**
             * Get the time of the measurement.
             *
             * @returns the time (in unix time) of the measurement.
             */
            time_t get_time() {
                struct tm  timestr;
                timestr.tm_sec = 0;
                timestr.tm_min = this->min;
                timestr.tm_hour = this->hour;
                timestr.tm_mon = this->month-1;
                timestr.tm_mday = this->day;
                timestr.tm_year = this->year+100;
                timestr.tm_wday = -1;
                timestr.tm_yday = -1;
                timestr.tm_isdst = -1;
                return mktime( &timestr );;
            }
            void set_time( time_t time ) {
                struct tm *timestr;
                timestr = localtime( &time );
                this->min = timestr->tm_min;
                this->hour = timestr->tm_hour;
                this->day = timestr->tm_mday;
                this->month = timestr->tm_mon+1;
                this->year = timestr->tm_year-100;
            }

            // Dump content of struct.
            void print( FILE *out = stdout ) {
                char buffer[1024];
                struct tm *timestr;
                time_t t = this->get_time();
                timestr = localtime( &t );
                strftime( buffer, 1024, "%c", timestr );
                fprintf( out, "%s %3d %3d '%s'\n", buffer, this->get_systolic(),
                         this->get_diastolic(),
                         WHO_class_str[this->get_classification()] );
            }
            void print_csv( FILE *out = stdout ) {
                fprintf( stdout,"\"%llu\",\"%d\",\"%d\",\"%d\"\n",
                         ( long long unsigned )this->get_time(),
                         this->get_systolic(),
                         this->get_diastolic(),
                         this->get_bpm() );
            }
            void print_txt( FILE *out = stdout ) {
                fprintf( out,"%llu %d %d\n",
                         ( long long unsigned )this->get_time(),
                         this->get_systolic(),
                         this->get_diastolic()
                       );
            }
    } measurement;

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
            sqlite3_stmt *list_last_stmt = nullptr;

            /**
             * Add the database definition.
             */
            void check_database() {
                const char *const statement =  "CREATE TABLE IF NOT EXISTS bpp ("
                                               "time INTEGER UNIQUE PRIMARY KEY,"
                                               "systolic INTEGER,"
                                               "diastolic INTEGER,"
                                               "bpm INTEGER);";

                char *errmsg = nullptr;
                int res = sqlite3_exec( this->handle, statement, nullptr, nullptr, &errmsg );

                if ( res != SQLITE_OK ) {
                    fprintf( stderr, "Failed to create table: %s\n", errmsg );
                }

            }

            /**
             * Precompile SQL statements.
             */
            void prepare_statements() {
                const char * const insert_db_str = "INSERT OR IGNORE INTO bpp VALUES(?,?,?,?);";
                int retv = sqlite3_prepare_v2( this->handle,
                                               insert_db_str, -1,
                                               &( this->insert_stmt ),nullptr );

                if ( retv != SQLITE_OK ) {
                    fprintf( stderr, "Failed to prepare statement: %s:%i\n", insert_db_str,retv );
                }

                const char * const list_all_str = "SELECT * from bpp ORDER BY time ASC";
                retv = sqlite3_prepare_v2( this->handle,
                                           list_all_str, -1,
                                           &( this->list_all_stmt ),nullptr );

                if ( retv != SQLITE_OK ) {
                    fprintf( stderr, "Failed to prepare statement: %s:%i\n", list_all_str,retv );
                }

                const char * const get_average_str = "SELECT avg(systolic), avg(diastolic) FROM bpp;";
                retv = sqlite3_prepare_v2( this->handle,
                                           get_average_str, -1,
                                           &( this->get_average_stmt ), nullptr );

                if ( retv != SQLITE_OK ) {
                    fprintf( stderr, "Failed to prepare statement: %s:%i\n", get_average_str,retv );
                }


                const char * const list_last_str = "SELECT * from bpp WHERE time >= ? ORDER BY time ASC ";
                retv = sqlite3_prepare_v2( this->handle,
                                           list_last_str, -1,
                                           &( this->list_last_stmt ),nullptr );

                if ( retv != SQLITE_OK ) {
                    fprintf( stderr, "Failed to prepare statement: %s:%i\n", list_last_str,retv );
                }
            }

            void open( const char *path ) {
                int res = sqlite3_open( path, &this->handle );

                if ( res != SQLITE_OK ) {
                    fprintf( stderr, "Failed to open database: %s\n", path );
                    delete path;
                    exit( 1 );
                    return;
                }

                check_database();
                prepare_statements();
            }

        public:
            void add( measurement &msg ) {
                sqlite3_bind_int64( this->insert_stmt, 1, msg.get_time() );
                sqlite3_bind_int( this->insert_stmt, 2, msg.get_systolic() );
                sqlite3_bind_int( this->insert_stmt, 3, msg.get_diastolic() );
                sqlite3_bind_int( this->insert_stmt, 4, msg.get_bpm() );
                sqlite3_step( this->insert_stmt );

                sqlite3_reset( this->insert_stmt );
            }

            std::tuple<uint8_t, uint8_t> average() {
                std::tuple<uint8_t, uint8_t> retv( 0,0 );

                int rc = sqlite3_step( this->get_average_stmt );

                switch ( rc )  {
                    case SQLITE_DONE:
                        break;

                    case SQLITE_ROW: {
                        std::get<0>( retv ) = sqlite3_column_int( this->get_average_stmt,0 );
                        std::get<1>( retv ) = sqlite3_column_int( this->get_average_stmt,1 );
                    }
                    break;

                    default:
                        fprintf( stderr, "Error iterating table: %s\n",
                                 sqlite3_errmsg( this->handle ) );
                        break;
                }

                // Reset the query.
                sqlite3_reset( this->list_all_stmt );
                return retv;
            }

            std::list<measurement>  list() {
                std::list<measurement> list;
                int rc;

                do {
                    rc = sqlite3_step( this->list_all_stmt );

                    switch ( rc )  {
                        case SQLITE_DONE:
                            break;

                        case SQLITE_ROW: {
                            measurement msg;
                            time_t time = sqlite3_column_int64( this->list_all_stmt,0 );
                            msg.set_time( time );
                            msg.set_systolic( sqlite3_column_int( this->list_all_stmt,1 ) );
                            msg.set_diastolic( sqlite3_column_int( this->list_all_stmt,2 ) );
                            msg.set_bpm( sqlite3_column_int( this->list_all_stmt,3 ) );
                            list.push_back( msg );
                        }
                        break;

                        default:
                            fprintf( stderr, "Error iterating table: %s\n",
                                     sqlite3_errmsg( this->handle ) );
                            break;
                    }

                } while ( rc == SQLITE_ROW );

                // Reset the query.
                sqlite3_reset( this->list_all_stmt );
                return list;
            }

            std::list<measurement>  list_since( time_t last ) {
                std::list<measurement> list;
                int rc;
                sqlite3_bind_int64( this->list_last_stmt, 1, last );

                do {
                    rc = sqlite3_step( this->list_last_stmt );

                    switch ( rc )  {
                        case SQLITE_DONE:
                            break;

                        case SQLITE_ROW: {
                            measurement msg;
                            time_t time = sqlite3_column_int64( this->list_last_stmt,0 );
                            msg.set_time( time );
                            msg.set_systolic( sqlite3_column_int( this->list_last_stmt,1 ) );
                            msg.set_diastolic( sqlite3_column_int( this->list_last_stmt,2 ) );
                            msg.set_bpm( sqlite3_column_int( this->list_last_stmt,3 ) );
                            list.push_back( msg );
                        }
                        break;

                        default:
                            fprintf( stderr, "Error iterating table: %s\n",
                                     sqlite3_errmsg( this->handle ) );
                            break;
                    }

                } while ( rc == SQLITE_ROW );

                // Reset the query.
                sqlite3_reset( this->list_last_stmt );
                return list;
            }


            Storage() {
                // Check if there is an override.
                if ( getenv( "BPM_PATH" ) == nullptr ) {
                    // Create path.
                    const char * filename = ".bpm.sqlite3";
                    const char * homedir = getenv( "HOME" );

                    if ( homedir == nullptr ) {
                        fprintf( stderr, "No 'HOME' directory set.\n" );
                        exit( -1 );
                    }

                    // Create memory with enough size to hold path.
                    ssize_t size = strlen( filename )+strlen( homedir )+2;
                    char *path = ( char * ) new char[size];
                    // Create path
                    snprintf( path, size, "%s/%s", homedir, filename );

                    open( path );

                    delete[] path;
                } else {
                    open( getenv( "BPM_PATH" ) );
                }
            }

            ~Storage() {
                if ( this->insert_stmt != nullptr ) {
                    sqlite3_finalize( this->insert_stmt );
                    this->insert_stmt = nullptr;
                }

                if ( this->list_all_stmt != nullptr ) {
                    sqlite3_finalize( this->list_all_stmt );
                    this->list_all_stmt = nullptr;
                }

                if ( this->list_last_stmt != nullptr ) {
                    sqlite3_finalize( this->list_last_stmt );
                    this->list_last_stmt = nullptr;
                }

                if ( this->get_average_stmt != nullptr ) {
                    sqlite3_finalize( this->get_average_stmt );
                    this->get_average_stmt = nullptr;
                }

                if ( this->handle != nullptr ) {
                    int res = sqlite3_close( this->handle );

                    if ( res != SQLITE_OK ) {

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

            void Read( int fd,uint8_t *buffer,ssize_t size ) {
                ssize_t d = 0;

                while ( d < size ) {
                    d += read( fd, &buffer[d], 1 );
                }
            }

            bool ping() {
                uint8_t msg = 0xAA;
                write( fd, &msg, 1 );

                read( fd, &msg, 1 );

                return ( msg == 0x55 );
            }
            void print_name() {
                uint8_t resp[33];
                uint8_t msg = 0xA4;

                write( fd, &msg, 1 );

                Read( fd,resp,32 );

                resp[32] = '\0';
                printf( "Found device: %s\n", resp );
            }

        public:
            int get_num_measurements() {
                uint8_t msg = 0xA2;
                write( fd, &msg, 1 );
                read( fd, &msg, 1 );
                return msg;
            }


            measurement get_measurement( int id ) {
                measurement retv;
                uint8_t msg = 0xA3;
                write( fd, &msg, 1 );
                msg = id+1;
                write( fd, &msg, 1 );

                Read( fd, ( uint8_t * )&retv, sizeof( measurement ) );

                return retv;
            }
            bool open_device() {
                struct termios newtio;
                const char *path  = getenv( "BPM_DEVICE" );

                // Fall back to default if not found.
                if ( path == nullptr ) {
                    path = MODEMDEVICE;
                }

                fd = open( path, O_RDWR | O_NOCTTY );

                if ( fd <0 ) {
                    perror( path );
                    return false;
                }

                /* save current serial port settings */
                tcgetattr( fd,&oldtio );

                /* clear struct for new port settings */
                memset( &newtio, 0,sizeof( newtio ) );
                /* configure port */
                newtio.c_cflag = BAUDRATE | CS8;
                newtio.c_iflag = 0;
                newtio.c_oflag = 0;
                newtio.c_lflag = 0;       //ICANON;
                newtio.c_cc[VMIN]=1;
                newtio.c_cc[VTIME]=0;

                /* set settings. */
                tcflush( fd, TCIFLUSH );
                tcsetattr( fd,TCSANOW,&newtio );

                if ( !ping() ) {
                    fprintf( stderr, "Failed to ping BPM58\n" );
                    return false;
                }

                print_name();
                return true;
            }
            void close_device() {
                if ( fd > 0 ) {
                    printf( "Closing link to BPM\n" );
                    /* restore the old port settings */
                    tcsetattr( fd,TCSANOW,&oldtio );
                    close( fd );
                    fd = 0;
                }
            }
        public:
            BM58() {}
            ~BM58() {
                close_device();
            }
    };


    class Main
    {
        private:
            Storage storage;
            BM58    bm58;
            bool filter = false;
            void import() {
                if ( bm58.open_device() ) {
                    int num_records = bm58.get_num_measurements();
                    printf( "Found: %03d records\n", num_records );

                    for ( int i = 0; i < num_records; i++ ) {
                        auto msg = bm58.get_measurement( i );
                        fputs( "\033[A\033[2K",stdout );
                        printf( "%03d/%03d\n", i+1, num_records );
                        storage.add( msg );
                    }
                }
            }
            /**
             *
             */
            void help() {
                printf( "BPM: Blood Pressure Monitor Client.\n" );
                printf( "Usage: bpm <option><command>\n" );
                printf( "Option:\n" );
                printf( "\tfilter:\tAverage multiple samples within 10 minutes in the next commands.\n" );
                printf( "Command:\n" );
                printf( "\tlist:\tList the measurements\n" );
                printf( "\tcsv:\tGenerate a CSV file for the measurements\n" );
                printf( "\ttxt:\tGenerate a TXT file for the measurements. Use for gnuplot.\n" );
                printf( "\timport:\tImport entries from  BPM.\n" );
                printf( "\tplot:\tCall gnuplot to plot the BPM.\n" );
                printf( "\tstatus:\tPrint status based on last measurements.\n" );
                printf( "\n" );
                printf( "Environments:\n" );
                printf( "\tBPM_DEVICE:\tDevice node to read samples from.\n" );
                printf( "\tBPM_PATH:\tDatabase file.\n" );
            }

            /**
             * list measurements
             */
            void list() {
                printf( "Listing nodes\n" );
                auto list = storage.list();

                if ( filter ) {
                    list = this->filter_list( list );
                }

                for ( auto it = list.begin(); it != list.end(); it++ ) {
                    ( *it ).print();
                }
            }
            void list_csv() {
                auto list = storage.list();

                if ( filter ) {
                    list = this->filter_list( list );
                }

                for ( auto it = list.begin(); it != list.end(); it++ ) {
                    ( *it ).print_csv();
                }
            }
            void list_txt() {
                auto list = storage.list();

                if ( filter ) {
                    list = this->filter_list( list );
                }

                for ( auto it = list.begin(); it != list.end(); it++ ) {
                    ( *it ).print_txt();
                }
            }

            /**
             * Print the average systolic, diastolic.
             */
            void print_avg() {
                auto val = storage.average();
                printf( "Average:\n\tSystolic: %d\n\tDiastolic: %d\n",
                        std::get<0>( val ), std::get<1>( val ) );
            }

            void plot( void ) {
                std::string gnuplot_file = std::string( PREFIX )+"/share/bpm/plot.gnuplot";
                int code = execlp( "gnuplot","gnuplot", gnuplot_file.c_str(), NULL );

                if ( code == -1 ) {
                    fprintf( stderr, "Failed to execute gnuplot: %s\n", strerror( errno ) );
                }
            }


            /**
             * All points within 'filter_range' time get merged.
             */
            const int filter_range = 60*10;

            /**
             * Filter the list.
             *
             * @param ls Reference to the list to filter.
             *
             * Samples within 10 minutes get merged and averaged.
             *
             * @returns a new std::list<measurement> with the filtered points.
             */
            std::list<measurement> filter_list ( const std::list<measurement> &ls ) {
                std::list<measurement> retv;

                // If empty, return empty list.
                if ( ls.empty() ) return retv;

                // Filter
                measurement last = *( ls.begin() );
                int elements = 1;

                for ( auto it : ls ) {
                    {
                        auto diff = it.get_time()-last.get_time();

                        if ( labs( diff ) < ( filter_range ) ) {
                            if ( it.get_diastolic() < last.get_diastolic() ) {
                                // Diastolic.
                                double dia = last.get_diastolic()*( elements/( double )( elements+1 ) );
                                dia+=it.get_diastolic()*( 1/( double )( elements+1 ) );
                                last.set_diastolic( dia );
                                // Systolic.
                                double sys = last.get_systolic()*( elements/( double )( elements+1 ) );
                                sys+=it.get_systolic()*( 1/( double )( elements+1 ) );
                                last.set_systolic( sys );
                                // BPM
                                double bpm = last.get_bpm()*( elements/( double )( elements+1 ) );
                                sys+=it.get_bpm()*( 1/( double )( elements+1 ) );
                                last.set_bpm( bpm );
                            }

                            elements++;
                        } else {
                            retv.push_back( last );
                            last = it;
                            elements = 1;
                        }
                    }
                }

                retv.push_back( last );
                return retv;
            }

            void status() {
                // Get last measurement.
                time_t now = time( nullptr );

                int days = 7;
                // Remove 5 days
                now -= 60*60*24*days;


                auto list = this->storage.list_since( now );

                if ( list.empty() ) {
                    printf( "No samples in the last %d days\n",days );
                    return;
                }

                WHO_class highest = OPTIMAL;
                unsigned int dia = 0;
                unsigned int sys = 0;
                measurement mes;

                for ( auto iter : list ) {
                    auto max = iter.get_classification();

                    if ( max < highest ) highest = max;

                    dia += iter.get_diastolic();
                    sys += iter.get_systolic();
                }

                dia /= list.size();
                sys /= list.size();
                mes.set_systolic( sys );
                mes.set_diastolic( dia );
                printf( "\n" );
                printf( "Highest WHO Classification: %s\n",
                        WHO_class_str[highest] );
                printf( "Average WHO Classification: %s\n",
                        WHO_class_str[mes.get_classification()] );
                printf( "Average Systolic:           %d\n", sys );
                printf( "Average Diastolic:          %d\n", dia );

            }

        public:
            Main() {}

            int run ( int argc, char *argv[] ) {
                for ( int i =1 ; i < argc; i++ ) {
                    if ( strncmp( argv[i], "import", 6 ) == 0 ) {
                        this->import();
                    } else if ( strncmp( argv[i], "list", 4 ) == 0 ) {
                        this->list();
                    } else if ( strncmp( argv[i], "csv", 3 ) == 0 ) {
                        this->list_csv();
                    } else if ( strncmp( argv[i], "txt", 3 ) == 0 ) {
                        this->list_txt();
                    } else if ( strncmp( argv[i], "avg", 3 ) == 0 ) {
                        this->print_avg();
                    } else if ( strncmp( argv[i], "filter", 6 ) == 0 ) {
                        this->filter = true;
                    } else if ( strncmp( argv[i], "plot", 4 ) == 0 ) {
                        this->plot();
                    } else if ( strncmp( argv[i], "status", 6 ) == 0 ) {
                        this->status();
                    } else {
                        this->help();
                        return EXIT_FAILURE;
                    }
                }

                if ( argc <2 ) {
                    this->help();
                    return EXIT_FAILURE;
                }

                return EXIT_SUCCESS;
            }
    };
}

int main( int Parm_Count, char *Parms[] )
{
    auto m = BPM::Main();

    return m.run( Parm_Count, Parms );
}
