Fiji/ImageJ Orthanc Plugin
==========================

[Link to the original post](https://groups.google.com/d/msg/orthanc-users/zX5BgdqhISo/a69FSFYqAwAJ)


Hi everyone,
We are pleased to announce the official release of 3 plugins to manage Orthanc services through Fiji/ImageJ.
These plugins have been fully developed by Anousone Vongsalat (student at IUT-Informatique-Toulouse)  during an internship of 11 weeks in the Hospitals of Toulouse (France) : IUCT-Oncopole and CHU-Toulouse-Rangueil


These 3 plugins are :

 * Anonymization : These plugin handle the anonymization of DICOMs stored in Orthanc with a design dedicated for efficiency and usability.
 * 200 dicoms Tags are taken account in the anonymization process and the settings are limited to only 6 options to choose from Keep or Clear some tags like date releated tags (date of acquisition, time...), patient body characteristics (Weight, height...), Date of birth etc.
 * Secondary captures DICOM could be reckognized and deleted automatically since the patient identificaiton could be hardprinted in the image and thoose SC usually inflate anonymized dicom without add value in research purposes.
 * Sharing services of anonymized DICOM : Generated anonymized dicom can be exported to a local zip, send to a DICOM AET, but also to a FTP, SFTP or WebDAV remote server (in one sigle click).
 * Anonymization tracking in CSV : The correspondency bettween original patient name and new anonymized name/ID can be generated in CSV.
 * Clinical Trial Processor : The anonymization name can be centralized in a database, for collaborative studies it is possible to define a list of expected patient name (by their birth-date) and to assign an anonymization name, each user will query the database to retrieve the predefined centralized anonymized name.

Additional documentation is available at http://petctviewer.org/index.php/feature/dicom-anonymization-and-sharing

This anonymization service will be fully ready when the issue 46 in the bug tracker will be fixed in Orthanc Server since some tags still remain in the Orthanc 1.2.0 version. 
We are in close connection with Orthanc/Osimis team and we want to publicly say that we REALLY enjoyed working with such a valuable team and really appreciate their support during our development.

 * Query/Retrieve : This plugin handles orthanc queries and retrieve through Fiji/ImageJ (Studies and series level).
Retrieve could be performed for one or multiple study/series in one request.
We also add an "History" option allowing you to search the history of a patient in one single click, the ID of the patient will be sent to a new request that you can point to your AET PACS to search for all previous studies 
of one selected patient.

 * Import : This plugin allows to recursively scan a whole folder (and subfolders) to send all dicom files to Orthanc server. This allow bulk import of large dataset of DICOMs to Orthanc server and is basically an Java translation of the Orthanc.py script import.


All these plugins are distributed with the free and open source GPL licence version 3.
The sources are available in Anousone's GitHub : https://github.com/anousv/
Compiled package with dependency is available from this direct link : https://github.com/anousv/OrthancAnonymization/raw/master/Release%20and%20Lib.zip


Feel free to use and extend this work, especially it should be dead easy to make a standalone java version of all these plugins to benefit from all these services outside Fiji/ImageJ.


These plugins are part of the Free and Open Source PET/CT viewer project and will be distributed in our Fiji Update site in the next release in few days (see http://petctviewer.org/)


We will be happy to hear from the community for comment, feedback and possible issues that could be met.


This will be our Orthanc contribution for this year and we hope to come back in one year with new ideas, development and software services on this amazing Orthanc-server ecosystem.


Best regards,

Anousone Vongsalat
Salim Kanoun
