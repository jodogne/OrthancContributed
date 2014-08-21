Contribution from Emsy Chan <emlscs@yahoo.com> on 2014-08-21
============================================================


I've managed to get create-dicom to accept an upload of an image file to be DICOMized! :)

Summary:
1. Make a POST to createDicom.php with PatientID, PatientName and image file to be uploaded (uploadDicom.html). (Be aware that php default max file upload size is 2Mb).
2. createDicom.php uses php_gd2 to convert incoming file to PNG, does a base64 encode, and passes it to orthanc/tools/create-dicom, together with PatientName and various DICOM tags.
3. create-dicom makes a new instance (#1) with auto-generated PatientID.
4. PatientID cannot be injected into create-dicom under Orthanc 0.8.2, so the auto-generated PatientID of instance (#1) is modified into the POSTed PatientID (from step 1) using instance/{id}/modify into another instance (#2)
5. Instance #2 is stored into orthanc by POSTing to /instance
6. Instance #1 is deleted

Issues: 
My hospital uses Siemens Syngo Imaging XS (an old PACs) which likely needs more DICOM tags defined before it will accept the DICOMized image. Will be looking into this.
However, my hospital also uses Syngo.Via and OSIRIX, which have no trouble accepting the DICOMized image with the tags I had already defined in the current createDicom.php.

Hope this helps anybody who needs it.

Regards,
Em
