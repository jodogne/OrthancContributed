<?php
//Requires php_curl and php_gd2
//Global params
$orthanc = "http://localhost:8042";

//Function for encode png to mime. I modified this to hard-select $_FILES.
function data_uri($mime) {  
	imagepng(imagecreatefromstring(file_get_contents($_FILES["file"]["tmp_name"])), "scanned.png");
	$contents = file_get_contents("scanned.png");
	$base64   = base64_encode($contents); 
	return ('data:' . $mime . ';base64,' . $base64);
	}

//Function to generate $data to be sent to curlSetOpt_PostFields. 
//This is similar to http_build_query, but I couldn't find a similar one for use with CURL, so I wrote my own.
function createCurlPostFieldsData($inputArray) {
	$string = "{";
	foreach ($inputArray as $tag => $value) {
		$string = $string . "\"" . $tag . "\":\"" . $value . "\",";
		}
	$string = rtrim($string, ',');
	$string = $string . "}";
	return $string;
	}

//Define the DICOM Tags & PNG File, build query
$patientID = $_POST['PatientID']; // unable to inject PatientID directly into orthanc 0.8.2, need to modify the new DICOM instance in another 2 more calls
$dicomTags = array (
	"PatientName" => $_POST['PatientName'],
	"SOPClassUID" => "1.2.840.10008.5.1.4.1.1.7", //SC
	"Modality" => "SC",
	"SeriesDescription" => "Consent Form",
	"StudyDate" => date(Ymd),
	"SeriesDate" => date(Ymd),
	"StudyTime" => "000000",
	"SeriesTime" => "000000",
	"PixelData" => data_uri('image/png')
	);
$data = createCurlPostFieldsData($dicomTags);

//Create a DICOM instance
$curl = curl_init();
curl_setopt ($curl, CURLOPT_URL, $orthanc . '/tools/create-dicom');
curl_setopt ($curl, CURLOPT_RETURNTRANSFER, 1);
curl_setopt ($curl, CURLOPT_POST, 1);
curl_setopt ($curl, CURLOPT_POSTFIELDS, $data);
$resp = curl_exec($curl);
curl_close($curl);

//The following 3 steps (replace, create and delete) can be removed once PatientID can be injected into create-dicom
//Replace PatientID and return modified DCM binary data
$elements = explode("\"", $resp);
$instance = str_replace("/instances/", "", $elements[7]);
$target = $orthanc . "/instances/" . $instance . "/modify";
$data = "{\"Replace\":{\"PatientID\":\"" . $patientID . "\"}}";

$curl = curl_init();
curl_setopt ($curl, CURLOPT_URL, $target);
curl_setopt ($curl, CURLOPT_RETURNTRANSFER, 1);
curl_setopt ($curl, CURLOPT_POST, 1);
curl_setopt ($curl, CURLOPT_POSTFIELDS, $data);
$resp = curl_exec($curl);
curl_close($curl);

//Create new instance with modified DCM binary data
$curl = curl_init();
curl_setopt ($curl, CURLOPT_URL, $orthanc . '/instances');
curl_setopt ($curl, CURLOPT_RETURNTRANSFER, 1);
curl_setopt ($curl, CURLOPT_POST, 1);
curl_setopt ($curl, CURLOPT_POSTFIELDS, $resp);
$resp = curl_exec($curl);
curl_close($curl);

//Delete initial (unchanged) instance
$deleteTarget = $orthanc . "/instances/" . $instance;
$curl = curl_init();
curl_setopt ($curl, CURLOPT_URL, $deleteTarget);
curl_setopt ($curl, CURLOPT_RETURNTRANSFER, 1);
curl_setopt ($curl, CURLOPT_POST, 1);
curl_setopt ($curl, CURLOPT_CUSTOMREQUEST, "DELETE");
$resp = curl_exec($curl);
curl_close($curl);

//Done
echo "Done";
?>
