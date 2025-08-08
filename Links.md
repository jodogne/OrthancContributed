Useful Links for Orthanc
========================

Mobile apps
-----------

 * ["Mobile DICOM viewers"](http://www.web3.lu/mobile-dicom-viewers/), post by Marco Barnig testing which mobile apps for radiology are compatible with Orthanc.


Docker
------

 * [**Official** container](https://hub.docker.com/u/jodogne/), and its [associated documentation](http://book.orthanc-server.com/users/docker.html) in the Orthanc Book.
 * [Osimis container](https://hub.docker.com/r/osimis/orthanc/), and its [associated documentation](https://osimis.atlassian.net/wiki/spaces/OKB/pages/26738689/How+to+use+osimis+orthanc+Docker+images).
 * [Cross-architecture orthanc container](https://hub.docker.com/r/derekmerck/orthanc/) and [Cross-architecture orthanc-plugins container](https://hub.docker.com/r/derekmerck/orthanc-plugins/) for `amd64`, `armhf`, and `aarch64`, and the [associated source code](https://github.com/derekmerck/docker-orthanc-xarch)
 * [Lightweight cross-architecture orthanc container](https://hub.docker.com/r/scratchcat1/orthanc/) and [Lightweight cross-architecture orthanc-plugins container](https://hub.docker.com/r/scratchcat1/orthanc-plugins/) and the [associated source code](https://github.com/Scratchcat1/OrthancDocker) with images built for `amd64` and `armhf`. Updated Debian base and images can be built for any architecture.
 
Kubernetes
----------

 * [Korthweb](https://github.com/digihunch/korthweb/) is a project by [Yi Lu](https://www.linkedin.com/in/digihunch/) for automated Orthanc deployment on Kubernetes platforms, with configurations for security and observability.

Ansible
-------

 * The ["GNU Health Automatic Deployment" Ansible playbooks](https://gitlab.com/geraldwiese/gnuhealth-automatic-deployment) by Gerald Wiese aims "to automatically deploy the hospital information system GNU Health including the HMIS node, Thalamus, the DICOM server Orthanc and a desktop environment as a client using the GNU Health HMIS node."

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
 * [orthanc-server-extensions](https://github.com/walkIT-nl/orthanc-server-extensions/): A simple Orthanc python plugin based framework to extend Orthanc’s feature set with testable python scripts
 * [Orthanc Plugin Scripts](https://github.com/mohannadhussain/orthanc-plugin-scripts): A collection of Orthanc plugins, written in Python, to add miscellaneous functionality, by [Mohannad Hussain](https://www.linkedin.com/in/mohannadhussain/).
 * [PyOrthanc](https://github.com/gacou54/pyorthanc): An open-source Python library that provides a comprehensive interface for interacting with Orthanc, by [Gabriel Couture](https://github.com/gacou54/).

Rust
----

 * [orthanc-cli](https://github.com/Ch00k/orthanc-cli) is a command-line interface for Orthanc by [Andrii Yurchuk](https://groups.google.com/g/orthanc-users/c/dfIV8IKLJNg/m/eQBGSEGACgAJ).
 * [orthanc-rs](https://github.com/Ch00k/orthanc-rs) is a Rust client library for the Orthanc REST API written by [Andrii Yurchuk](https://groups.google.com/g/orthanc-users/c/dfIV8IKLJNg/m/eQBGSEGACgAJ).

PHP
---

 * [PHP API Client for Orthanc](https://github.com/aurabx/orthancphp) by [christopher.skene](https://discourse.orthanc-server.org/t/php-client-library/5249).

Cloud
-----

 * [OrthWeb](https://github.com/digihunch/orthweb/) is a project by by [Yi Lu](https://www.linkedin.com/in/digihunch/) for automated Orthanc deployment on AWS. It  provisions underlying cloud resources and configures Docker to host Orthanc on EC2 instance.
 * [Orthanc deployment on AWS using AWS CDK](https://github.com/aws-samples/orthanc-cdk-deployment) is a project that "aims to help you provision a ready-to-use Orthanc cluster on Amazon ECS Fargate, with support for the official S3 plugin." It is written by [Tamas Santa](https://groups.google.com/g/orthanc-users/c/kpWCpplqI00/).
 * [Research PACS on AWS](https://github.com/aws-samples/research-pacs-on-aws) is a project that "facilitates researchers to access medical images stored in the clinical PACS in a secure and seamless manner, after potentially identifying information is removeed, and allows medical images to be exported to Amazon S3 in order to leverage cloud capabilities for processing." It is written by [Nicolas Malaval](https://groups.google.com/g/orthanc-users/c/WqcbI8yGDSs/).


Tools based upon Orthanc
------------------------

 * [DICOM tools built on Orthanc API in Java](https://github.com/salimkanoun/Orthanc_Tools)
   by Salim Kanoun, from the Free and Open Source
   [PET/CT viewer project](http://petctviewer.org/).
 
   * The GUI tool to edit the JSON configuration file of Orthanc that
     was previously a separated project is now part of the PET/CT
     viewer project.
 
 * [Upload and import study (files or zip file) to Orthanc](https://groups.google.com/forum/#!msg/orthanc-users/LHL4bsLDYP8/w2_iDtwhEgAJ;context-place=topic/orthanc-users/wwCII2uZDcQ)
   by Rana Asim Wajid.

 * [RadioLogic](https://github.com/mbarnig/RadioLogic) by Marco Barnig.  RadioLogic is a case-based learning and self-assessment tool for the Orthanc Ecosystem for Medical Imaging.
 
 * [Orthanc Paper Scanner in Apache PHP](https://sourceforge.net/projects/orthanc-paperscanner-apachephp/). This project is "a paper scanning portal that gives the Orthanc Pacs server the ability to interface with twain/wia scanners and insert the resulting pdf into the Orthanc database using a web interface integrated into the Orthanc web console". Check out the [original announcement on the Orthanc Users discussion group](https://groups.google.com/forum/?utm_medium=email&utm_source=footer#!msg/orthanc-users/k0CQ1f5eu84/z98izDJ8AQAJ).

 * https://github.com/sscotti/PACS_Integration

 * [Longitudinal Anonymization](https://github.com/HDVUCAIR/OrthancContribution).  A collection of bash, Lua, and Python scripts to launch a constellation of Orthanc/PostGreSQL servers using docker-compose for performing longitudinal anonymization of patient/studies.
   * Studies are internally consistent in that all UID are replaced with anonymized UID, but references/links between series are retained.
   * Relative times between studies for the same patient are retained, but the dates are randomly shifted up to 365 days.  One random shift is generated per patient for all their studies.

External documentations and user guides
---------------------------------------
   
 * [Swagger / OpenAPI specification for Orthanc](https://github.com/bastula/swagger-orthanc), draft by Aditya Panchal. [Link to forum](https://groups.google.com/d/msg/orthanc-users/y1t3XIc7aIc/AF-vy6cdEgAJ).
   * 2020-12-30: This contribution is superseded by the [official OpenAPI](https://api.orthanc-server.com/) that is generated directly from the source code of Orthanc.
 * [Extending Orthanc with Postresql database](http://petctviewer.org/images/Extending_Orthanc_with%20_PostgreDB.pdf) by Ilan Tal – Salim Kanoun
 * [DICOM connection setup user guide](http://www.petctviewer.org/images/QuickSetupGuide_Networking_DICOM.pdf) by Ilan Tal – Salim Kanoun
 * [Orthanc setup on Synology](https://github.com/levinalex/orthanc-synology-ansible-example) using Ansible, by Levin Alexander.
 * [RadioLogic - wiki](https://github.com/mbarnig/RadioLogic/wiki) by Marco Barnig.  Tons of useful info about how to use Orthanc, compile it and how to develop Orthanc plugins.
