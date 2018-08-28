# Orthanc_Tools
DICOM tools built on Orthanc API in Java

Detailled Documentation :  see Orthanc_Tools_Documentation.pdf, Anonymization_Guideline.pdf
Compiled standalone app : https://github.com/salimkanoun/Orthanc_Tools/releases

Features : 

- Anonymization : Fine tunable anonymization and sharing services of Anonymized DICOMs (FTP/SSH/WebDAV)

- Modification : Edit DICOM tags in Patient / Study / Serie levels

- Export : 
   - Zip Export of DICOMs stored in Orthanc (Hierachical or DICOMDIR)
   - CD/DVD image generation with patient's DICOM and ImageJ viewer (zip or ISO)
   
 - Manage : 
   - Single and Batch deletion of Patients / Studies / Series in Orthanc
   
 - Query : 
   - Query / Retrieve from remote AET
   - Automatic / Batch retrieve of studies (with Schedule feature)
      - Possibility to make series based filters for selective auto - retrieve
      - CSV report of auto-retrieve procedure
   
 - Import :
   - Recursive import to Orthanc of local DICOMs
   
 - Monitoring :
   - Auto-Fetch : Automatically retrieve patient's history for each new study/patient recieved in Orthanc
   - CD-Burner : Generate DVD burning intruction for Epson PP100II diskmaker
   - Tag-Monitoring : Autocollection of DICOM tag value of recieved patients/studies/series (monitoring injected dose, patient's weight, acquisition time ...) and possibility to store these data into a structured mySQL database
   - Auto-Routing (Not desgigned yet)
   
 - Orthanc JSON Editor :  
   - Edit all Orthanc settings in a comprehensive GUI and generate JSON settings file for Orthanc Server
   
 Contribution from http://petctviewer.org, free and open source PET/CT viewer based on Fiji/ImageJ
 
 GPL v.3 Licence
 
 Salim Kanoun & Anousone Vongsalat
 