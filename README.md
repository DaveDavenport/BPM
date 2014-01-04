BPM(2014-1-4)                                                    BPM(2014-1-4)



NNAAMMEE
       BPM  -  Small  command-line  tool  to store data from Beurer BM58 Blood
       Pressure Monitor

       bbppmm [_c_o_m_m_a_n_d_s]


DDEESSCCRRIIPPTTIIOONN
       This manual page explains the BBPPMM program. The  BBPPMM  program  retrieve,
       store  and  output blood pressure measurements from a Beurer BM56 Blood
       Pressure Monitor.  (Any device based on   Andon  Blood  Pressure  Meter
       KD001 should work).

       Be  default  these  devices  can only store up to 60 data points.  This
       tool is written to extend that and allow the data to  be  used  by  3rd
       party programs like gnuplot.  It outputs the stored points in different
       formats like txt and csv, and if requested,  filter  multiple  measure‐
       ments taken at the same time.



CCOOMMMMAANNDDSS
       Multiple  commands  can  be  specified  in  one go, they are handled in
       order.


       iimmppoorrtt Import data from the Blood Pressure Monitor into the local data‐
              base.


       lliisstt   List all the logged data points.


       ccssvv    Output the logged data points in Comma Separated Value format.


       ttxxtt    Output  the logged data points in simple text format.  This for‐
              mat can be used directly in gnuplot.


       aavvgg    Display average Systolic and Diastolic preasure.


       ffiilltteerr Measurements taken within 10 minutes of each other are averaged.





EEXXAAMMPPLLEESS
       bbppmm iimmppoorrtt ffiilltteerr ccssvv

       Imports the latest samples from the Blood Pressure Monitor and output a
       filtered comma separated value list.


EENNVVIIRROONNMMEENNTT VVAARRIIAABBLLEESS
       BPM obeys the following environment variables:


       BBPPMM__PPAATTHH
              The full path to the database file.
              _D_e_f_a_u_l_t $(HOME)/.bpm.sqlite3


       BBPPMM__DDEEVVIICCEE
              The device node pointing to the serial device of the Bloop Pres‐
              sure Monitor
              _D_e_f_a_u_l_t /dev/ttyUSB0



SSUUPPPPOORRTTEEDD DDEEVVIICCEESS
       The following device(s) are known to work:

       BBeeuurreerr mmeeddiiccaall BBMM5588



BBUUGGSS
       Please report bugs on the GITHUB issues page:  https://github.com/Dave‐
       Davenport/bpm


NNOOTTEESS
       The  program  is available as-is. It is not a medical-grade program and
       should not be used for this purpose.


SSEEEE AALLSSOO
gnuplot(1)                                                       BPM(2014-1-4)
