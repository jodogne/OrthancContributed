Contribution from Emsy Chan <emlscs@yahoo.com> on 2014-08-19
============================================================


Hi,

I feel that orthanc is a great mini-PACS and have an urge to contribute something. However, please forgive my very limited programming skills. 

I want to share my (very basic) method of implementing Weasis with Orthanc using PHP and XML.

Summary: 
1. Orthanc Patient UUID is passed to loadWeasisX.php (eg http://webserver:80/pacs/loadWeasisX.php?PatientUUID=2c713ae2-c924aea4-c86e9306-f7873fb5-fd8444d0)
2. loadWeasisX.php using file_get_contents from http://orthanc:8042/, parses the data using raw PHP, and writes a Weasis-compatible XML file (weasisX.xml)
3. loadWeasisX.php returns a link to weasisX.jnlp, which is hard-coded to load weasisX.xml

Note: 
1. The local addresses in the files attached are 10.1.20.127 (webserver with PHP) and 10.2.150.33:8042 (orthanc with default 8042 port for webserver).
2. Weasis is installed into the webserver/pacs/weasis (which is where the jnlp points to).

Hope this helps with anybody who needs it.

Regards,
Em
