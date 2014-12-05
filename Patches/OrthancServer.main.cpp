--- main.cpp	2014-09-19 11:46:51.000000000 +0300
+++ main.updated.cpp	2014-12-05 16:05:21.000000000 +0200
@@ -681,6 +681,12 @@
   {
     LOG(ERROR) << "Uncaught exception, stopping now: [" << e.What() << "]";
     status = -1;
+  } catch (const std::exception& e) {
+    LOG(ERROR) << "Exception: [" << e.what() << "]";
+    status = -1;
+  } catch (const std::string& s) {
+    LOG(ERROR) << "Exception: [" << s << "]";
+    status = -1;
   }
   catch (...)
   {
