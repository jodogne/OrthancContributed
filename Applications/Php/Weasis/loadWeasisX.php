<?php
$orthancAddress = "http://10.2.150.33:8042";

//get and fill in patients
$patient = file_get_contents($orthancAddress . "/patients/" . $_GET['PatientUUID']);
$studies = $patient; //pass to next part to get studies
$patientTags = explode("\n", $patient);

for ($p = 0; $patientTags[$p] != ""; $p++) {
	if (strpos($patientTags[$p], "\"PatientName\"") !== false) {
		$patientNameTags = explode ("\"", $patientTags[$p]);
		$patientName = $patientNameTags[3];
		}
	if (strpos($patientTags[$p], "\"PatientID\"") !== false) {
		$patientIDTags = explode ("\"", $patientTags[$p]);
		$patientID = $patientIDTags[3];
		}
	if (strpos($patientTags[$p], "\"PatientBirthDate\"") !== false) {
		$patientBirthDateTags = explode ("\"", $patientTags[$p]);
		$patientBirthDate = $patientBirthDateTags[3];
		}
	}

$xmlText = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<wado_query xmlns= \"http://www.weasis.org/xsd\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" wadoURL=\"" . $orthancAddress . "/instances/\" requireOnlySOPInstanceUID=\"false\" additionnalParameters=\"\" overrideDicomTagsList=\"\" >\n  <Patient PatientID=\"" . $patientID . "\" PatientName=\"" . $patientName . "\" PatientBirthDate=\"" . $patientBirthDate . "\" >\n";

// get and fill study & series (nested loop)
$cropStart = strpos($studies, "[");
$cropEnd = strpos($studies, "]");
$studies = substr($studies, $cropStart + 3, $cropEnd - $cropStart - 5);

$studies = str_replace("\"", "", $studies);
$studies = str_replace("\n", "", $studies);
$studies = str_replace(" ", "", $studies);
$study = explode(",", $studies);

for ($s = 0; $study[$s] != ""; $s++){
	$series = file_get_contents($orthancAddress . "/studies/" . $study[$s]);

	$studyTags = explode("\n", $series);
	for ($p = 0; $studyTags[$p] != ""; $p++) {
		if (strpos($studyTags[$p], "\"StudyInstanceUID\"") !== false) {
			$studyUIDTags = explode ("\"", $studyTags[$p]);
			$studyUID = $studyUIDTags[3];
			}
		if (strpos($studyTags[$p], "\"StudyDescription\"") !== false) {
			$studyDescriptionTags = explode ("\"", $studyTags[$p]);
			$studyDescription = $studyDescriptionTags[3];
			}
		if (strpos($studyTags[$p], "\"StudyDate\"") !== false) {
			$studyDateTags = explode ("\"", $studyTags[$p]);
			$studyDate = $studyDateTags[3];
			}
		if (strpos($studyTags[$p], "\"StudyTime\"") !== false) {
			$studyTimeTags = explode ("\"", $studyTags[$p]);
			$studyTime = $studyTimeTags[3];
			}
		}
	$xmlText = $xmlText . "    <Study StudyInstanceUID=\"" . $studyUID . "\" StudyDescription=\"" . $studyDescription . "\" StudyDate=\"" . $studyDate . "\" StudyTime=\"" . $studyTime . "\" >\n";

	$cropStart = strpos($series, "[");
	$cropEnd = strpos($series, "]");
	$series = substr($series, $cropStart + 3, $cropEnd - $cropStart - 5);

	$series = str_replace("\"", "", $series);
	$series = str_replace("\n", "", $series);
	$series = str_replace(" ", "", $series);
	$seri = explode(",", $series);

	for ($t = 0; $seri[$t] != ""; $t++){
		$instances = file_get_contents($orthancAddress . "/series/" . $seri[$t]);

		$seriesTags = explode("\n", $instances);
		for ($p = 0; $seriesTags[$p] != ""; $p++) {
			if (strpos($seriesTags[$p], "\"SeriesInstanceUID\"") !== false) {
				$seriesUIDTags = explode ("\"", $seriesTags[$p]);
				$seriesUID = $seriesUIDTags[3];
				}
			if (strpos($seriesTags[$p], "\"SeriesDescription\"") !== false) {
				$seriesDescriptionTags = explode ("\"", $seriesTags[$p]);
				$seriesDescription = $seriesDescriptionTags[3];
				}
			if (strpos($seriesTags[$p], "\"SeriesNumber\"") !== false) {
				$seriesNumberTags = explode ("\"", $seriesTags[$p]);
				$seriesNumber = $seriesNumberTags[3];
				}
			if (strpos($seriesTags[$p], "\"Modality\"") !== false) {
				$modalityTags = explode ("\"", $seriesTags[$p]);
				$modality = $modalityTags[3];
				}
			}
		$xmlText = $xmlText . "      <Series SeriesInstanceUID=\"" . $seriesUID . "\" SeriesDescription=\"" . $seriesDescription . "\" SeriesNumber=\"" . $seriesNumber . "\" Modality=\"" . $modality . "\" >\n";

		$cropStart = strpos($instances, "[");
		$cropEnd = strpos($instances, "]");
		$instances= substr($instances, $cropStart + 3, $cropEnd - $cropStart - 5);
	
		$instances= str_replace("\"", "", $instances);
		$instances= str_replace("\n", "", $instances);
		$instances= str_replace(" ", "", $instances);
		$instance = explode(",", $instances);
	
		for ($u = 0; $instance[$u] != ""; $u++) {
			$instanceList = file_get_contents($orthancAddress . "/instances/" . $instance[$u]);
			$instanceListTags = explode("\n", $instanceList);
			for ($p = 0; $instanceListTags[$p] != ""; $p++) {
				if (strpos($instanceListTags[$p], "\"SOPInstanceUID\"") !== false) {
					$instanceUIDTags = explode ("\"", $instanceListTags[$p]);
					$instanceUID = $instanceUIDTags[3];
					}
				if (strpos($instanceListTags[$p], "\"InstanceNumber\"") !== false) {
					$instanceNumberTags = explode ("\"", $instanceListTags[$p]);
					$instanceNumber = $instanceNumberTags[3];
					}
				}
			$xmlText = $xmlText . "        <Instance SOPInstanceUID=\"" . $instanceUID . "\" InstanceNumber=\"" . $instanceNumber . "\" DirectDownloadFile=\"" . $instance[$u] . "/file\"/>\n";
			}
		$xmlText = $xmlText . "</Series>\n";
		}
	$xmlText = $xmlText . "</Study>\n";
	}
$xmlText = $xmlText . "</Patient>\n</wado_query>\n";

$fp = fopen('weasisX.xml', 'w');
fwrite($fp, $xmlText);
fclose($fp);

?>

<script type="text/javascript">
function loadAndClose() {
	window.location = "http://10.1.20.127/pacs/weasisX.jnlp";
	}
</script>

<html>
<body onLoad="loadAndClose()">
<a href="javascript:window.close()">Close Window</a>
</body>
</html>
