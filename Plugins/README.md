# Contributed Orthanc plugins

## General information

Orthanc plugins must use the [plugin SDK](https://code.google.com/p/orthanc/source/browse/Plugins/Include/OrthancCPlugin.h) and must be written in C or C++. They must also fullfil the terms of the [GPLv3 license](http://www.gnu.org/licenses/quick-guide-gplv3.en.html). Sample code for plugins can be found [in the official Orthanc repository](https://code.google.com/p/orthanc/source/browse/?name=default#hg%2FPlugins%2FSamples) (in the `Plugins/Samples` folder of the `plugins` branch). A tutorial showing how to implement a basic WADO server is [available on CodeProject](http://codeproject.com/Articles/797118/Implementing-a-WADO-Server-using-Orthanc).

## Database

* [PostgreSQL](https://code.google.com/p/orthanc-postgresql/): Two **official** plugins to replace the default storage area (on the filesystem) and the default SQLite index by PostgreSQL.

## Viewers

* [Orthanc Web Viewer](https://code.google.com/p/orthanc-webviewer/): This *official* plugin extends Orthanc with a Web viewer of medical images.
* [DwvExplorer](https://github.com/ivmartel/DwvExplorer): This plugin is based on [DWV](https://github.com/ivmartel/dwv/wiki) by Yves Martel.

## HTML

* [ServeFolders](https://code.google.com/p/orthanc/source/browse/?name=default#hg%2FPlugins%2FSamples%2FServeFolders): This **official** plugin enables Orthanc to serve additional folders with its embedded Web server.
