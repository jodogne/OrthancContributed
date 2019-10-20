Useful Links for Orthanc
========================

Mobile apps
-----------

 * ["Mobile DICOM viewers"](http://www.web3.lu/mobile-dicom-viewers/), post by Marco Barning testing which mobile apps for radiology are compatible with Orthanc.


Docker
------

 * [**Official** container](https://hub.docker.com/u/jodogne/), and its [associated documentation](http://book.orthanc-server.com/users/docker.html) in the Orthanc Book.
 * [Osimis container](https://hub.docker.com/r/osimis/orthanc/), and its [associated documentation](https://osimis.atlassian.net/wiki/spaces/OKB/pages/26738689/How+to+use+osimis+orthanc+Docker+images).
 * [Cross-architecture orthanc container](https://hub.docker.com/r/derekmerck/orthanc/) and [Cross-architecture orthanc-plugins container](https://hub.docker.com/r/derekmerck/orthanc-plugins/) for `amd64`, `armhf`, and `aarch64`, and the [associated source code](https://github.com/derekmerck/docker-orthanc-xarch)
 
 
Vagrant
-------

 * [By Chris Hafey](https://github.com/chafey/orthanc-vagrant)
 * [By Fernando Jose Serrano Garcia](https://github.com/fernandojsg/vagrant-orthanc)

Plugins
-------

 * [Orthanc-Explorer 2](https://github.com/sdoloris/orthanc-explorer-2) provided by S. Doloris, R. Mathonet, R. Pire, P. Mertes and M. Horman (students at [ULiège](https://uliege.be), Belgium)
 * [Mongo-DB](https://github.com/Doc-Cirrus/orthanc-mongodb) provided by [Doc-Cirrus](https://www.doc-cirrus.com).
 * [VPI Reveal](https://github.com/jodogne/OrthancContributed/tree/master/Plugins/orthancVPIRevealPlugin) provided by [VPI-reveal](http://www.vpireveal.com/) is  a plugin that stores the files on disk in a more friendly tree (Patient/Study/Series/Instance).

Node.js
-------

 * [Orthanc Client](https://github.com/FWoelffel/orthanc-client) by Frédéric Woelffel ("Orthanc REST API client designed for NodeJS").
 
Ruby and Ruby on Rails
----------------------

 * [Ruby gem](https://rubygems.org/gems/orthanc/versions/0.1.0) by Simon Rascovsky ("Client for the Orthanc DICOM server REST API").
 
 Python
 ------
 
 * [beren](https://pypi.org/project/beren/): Orthanc REST client in Python

Tools based upon Orthanc
------------------------

 * [DICOM tools built on Orthanc API in Java](https://github.com/salimkanoun/Orthanc_Tools)
   by Salim Kanoun, from the Free and Open Source
   [PET/CT viewer projet](http://petctviewer.org/).
 
   * The GUI tool to edit the JSON configuration file of Orthanc that
     was previously a separated project is now part of the PET/CT
     viewer project.
 
 * [Upload and import study (files or zip file) to Orthanc](https://groups.google.com/forum/#!msg/orthanc-users/LHL4bsLDYP8/w2_iDtwhEgAJ;context-place=topic/orthanc-users/wwCII2uZDcQ)
   by Rana Asim Wajid.


External documentations and user guides
---------------------------------------

 * [Swagger / OpenAPI specification for Orthanc](https://github.com/bastula/swagger-orthanc), draft by Aditya Panchal. [Link to forum](https://groups.google.com/d/msg/orthanc-users/y1t3XIc7aIc/AF-vy6cdEgAJ).
 * [Extending Orthanc with Postresql database](http://petctviewer.org/images/Extending_Orthanc_with%20_PostgreDB.pdf) by Ilan Tal – Salim Kanoun
 * [DICOM connection setup user guide](http://www.petctviewer.org/images/QuickSetupGuide_Networking_DICOM.pdf) by Ilan Tal – Salim Kanoun
 * [Orthanc setup on Synology](https://github.com/levinalex/orthanc-synology-ansible-example) using Ansible, by Levin Alexander.

