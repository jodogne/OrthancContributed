<script>
var sliceUUID = []; //array to hold the instance UUIDs. The index of the elements is based on instanceNumber because Orthanc does not list the instances according to instanceNumber. Assumption that slice position correlates with instanceNumber.
<?php
$orthancAddress = "http://localhost:8042";

//get list of instances from Orthanc into $instance[]
$instances = file_get_contents($orthancAddress . "/series/" . $_GET['series'] . "/instances"); //uses 0.8.3
$seriesTags = explode("\n", $instances);
//$u = 0;
for ($i = 0; $seriesTags[$i] != ""; $i++) {
	$tags = explode(":", $seriesTags[$i]);
	if (strpos($tags[0], "IndexInSeries") !== false) {
		$instanceNumber = trim($tags[1],' ,');
		$prevTags = explode(":", $seriesTags[$i - 1]); //UUID is before IndexInSeries
		$UUID = trim($prevTags[1],' ",');

//1 worker, multiple calls
//		$sliceNumberUUID[$u] = $instanceNumber . "," . $UUID;
//		$u++;

//odd + even
		if (($instanceNumber / 2) == round($instanceNumber / 2)) {
			if (($instanceNumber / 4) == round($instanceNumber / 4)) {
				$sliceNumberUUIDEvenTwo = $sliceNumberUUIDEvenTwo . $instanceNumber . "," . $UUID . ","; 
				} else {
				$sliceNumberUUIDEven = $sliceNumberUUIDEven . $instanceNumber . "," . $UUID . ","; 
				}
			} else {
			if ((($instanceNumber + 1) / 4) == round(($instanceNumber + 1) / 4)) {
				$sliceNumberUUIDOddTwo = $sliceNumberUUIDOddTwo . $instanceNumber . "," . $UUID . ","; 
				} else {
				$sliceNumberUUIDOdd = $sliceNumberUUIDOdd . $instanceNumber . "," . $UUID . ","; 
				}
			}
	
//original
//		$sliceNumberUUID = $sliceNumberUUID . $instanceNumber . "," . $UUID . ","; //sliceNumberUUID is a CSV that contains the instanceNumber,instanceUUID,etc to be passed to the webworker
		echo "sliceUUID[" . $instanceNumber . "] = \"" . $UUID . "\";\n";		
		}
	}
?>
</script>

<?php
//for some reason, cannot use /content/ to get rows/cols - have to strip image width/height from PNG file, so get from last UUID
$pngFile = bin2hex(file_get_contents($orthancAddress . "/instances/" . $UUID . "/image-uint16"));
$posIHDR = strpos($pngFile, "49484452");
$width = hexdec(substr($pngFile, $posIHDR + 8, 8));
$height = hexdec(substr($pngFile, $posIHDR + 16, 8));

//for performance reason, limit size to below 1000 pixels - this matches the image resize in pngToGray.php
if ($width > 2000 || $height > 2000) {
	$width = $width / 4;
	$height = $height / 4;
	}
if ($width > 1000 || $height > 1000) {
	$width = $width / 2;
	$height = $height / 2;
	}

//get patient information
$patientName = file_get_contents($orthancAddress . "/instances/" . $UUID . "/content/0010-0010");
$patientID = file_get_contents($orthancAddress . "/instances/" . $UUID . "/content/0010-0020");
$studyDesc = file_get_contents($orthancAddress . "/instances/" . $UUID . "/content/0008-1030");

echo "<title>mcdcm - " . $patientName . "</title>\n";
?>

<body bgcolor="#111111"><center><font face="calibri" color="#ffffff">

<?php
echo "<font size=+1>" . $patientName . " " . $patientID . "</font><br>\n";
echo $studyDesc . "<br>\n";
echo "<canvas id=\"myCanvas\" height=" . $height . " width=" . $width . "></canvas><br>\n";
?>

<output id="imageinfo"></output><br>
<output id="loadstatusodd"></output><output id="loadstatuseven"></output><output id="loadstatusoddtwo"></output><output id="loadstatuseventwo"></output><output id="loadstatus"></output><br>

<!--uses jsxcompressor "ungzip" image data-string passed back from pngToGray.php-->
<script src="jsxcompressor.min.js" type="text/javascript"></script>

<script type="text/javascript">
var canvas = document.getElementById('myCanvas');
var canvasWidth = canvas.width;
var canvasHeight = canvas.height;
var ctx = canvas.getContext('2d');

//from http://stackoverflow.com/questions/14446447/javascript-read-local-text-file
function getGrayFromPNG(file)
{
    var rawFile = new XMLHttpRequest();
    rawFile.open("GET", file, false);
    rawFile.onreadystatechange = function ()
    {
        if(rawFile.readyState === 4)
        {
            if(rawFile.status === 200 || rawFile.status == 0)
            {
                allText = rawFile.responseText;
		}
        }
    }
    rawFile.send(null);
    return(JXG.decompress(allText));
}

//draws on the canvas based on direct call or from adjust() function
function drawSlice(s, windowCenterAdj, windowWidthAdj, panX, panY, zoom) {

	var canvasData = ctx.getImageData(0, 0, canvasWidth, canvasHeight);
	
	//web worker should already have filled in the sliceStream[] array with image data-string, but this line will load the image data-string from pngToGray if sliceStream happens to empty. This is used in the loading the 1st instance.
	if (sliceStream[s] == null) {
		sliceStream[s] = (getGrayFromPNG('pngToGray.php?instance=' + sliceUUID[s]));		
		}
	var pixels = sliceStream[s].split(','); //sliceStream[s] is CSV of pixelData

	baseX = baseX + panX; // for panning
	baseY = baseY + panY; 
	
	//my simple windowing algorithm because image data-string is 12-bit. HTML5 canvas is 8-bit (grayscale). Algorithm needed to parse 12-bit depth into 8-bit depth. An approximate windowCenter adjustment of 1020 to match CT HUs.
	windowCenter = windowCenter + windowCenterAdj;
	windowWidth = windowWidth + windowWidthAdj; 
	var low = (windowCenter + 1020) - (windowWidth / 2); 
	var high = (windowCenter + 1020) + (windowWidth / 2); 
	var grad = (high - low) / 255;

	var p = 0;

	for (var i = 0; i < height; i++) {
		for (var j = 0; j < width; j++) { 
			var pix = pixels[p];
			if (pix < low) { pix = 0; }
			if (pix >= low && pix <= high) {
				pix = (pix - low) / grad;
				}
			if (pix > high) { pix = 255; }
			pix = Math.round(pix);
			var pixDec = pix;
		    var index = (j + i * canvasWidth) * 4;
		    canvasData.data[index + 0] = pixDec;
		    canvasData.data[index + 1] = pixDec;
		    canvasData.data[index + 2] = pixDec;
		    canvasData.data[index + 3] = 255;
			p++;
			}
		}
	if (panX != 0 || panY != 0) {
		ctx.fillRect(0,0,canvas.width,canvas.height); //erases before pan
		}
	ctx.putImageData(canvasData, baseX, baseY); //draws image in canvas. X,Y based on panX and panY
	document.getElementById('imageinfo').innerHTML = "Instance: " + s + " Center: " + windowCenter + " Width: " + windowWidth + " ";
	}

//function to send adjustment data to drawSlice. Can actually be inserted into drawSlice(), but for clarity sake I separated it
function adjust(s, windowCenterAdj, windowWidthAdj, panX, panY, zoom) {
	if (currentSlice + s >= 1 && currentSlice + s <= lastSlice) {
		currentSlice = currentSlice + s;
		drawSlice(currentSlice, windowCenterAdj, windowWidthAdj, panX, panY, zoom);
		} 
	}

//Code starts running here
//baseline variable declaration
var windowCenter = 40;
var windowWidth = 80;
var sliceStream = []; //CSV of pixelData retrieved from pngToGray.php
var currentSlice = 1;
var lastSlice = 1;

<?php
echo "var height = " . $height . ";\n";
echo "var width = " . $width . ";\n";
?>

//var height = 512;
//var width = 512;
var baseX = 0;
var baseY = 0;
var panX = 0;
var panY = 0;
var zoom = 0; // zoom doesn't work yet

//draw the first instance (currentSlice = 1)
drawSlice(currentSlice,0,0,0,0,0);

/* //1 worker, multiple calls
//create a new webworker to load the image data-strings based on the instanceUUIDs from pngToGray and passes it back to main script
var w1;
if(typeof(w) == "undefined") {
    w1 = new Worker("bgLoadOne.js");
}

<?php
//for ($u = 0; $sliceNumberUUID[$u] != ""; $u++) {
//	echo "w1.postMessage('" . $sliceNumberUUID[$u] . "');\n";
//	}
?>
//w1.postMessage(sliceNumberUUID); //send the instanceNumbers and UUIDs to the webworker

//when webworker manages to load each UUID into pngToGray, it will postMessage the compressed image data-string back to main script
w1.onmessage = function(event){
	var workerData = event.data.split(','); //CSV again, [0] is instance number, [1] is compressed image data-string
	sliceStream[workerData[0]] = JXG.decompress(workerData[1]); 
	lastSlice = workerData[0];
	document.getElementById("loadstatus").innerHTML = "Loading: " + workerData[0] + "/" + (sliceUUID.length - 1);
};
*/

//Odd + Even
//create a new webworker to load the image data-strings based on the instanceUUIDs from pngToGray and passes it back to main script
var wo;
if(typeof(wo) == "undefined") {
    wo = new Worker("bgLoadOdd.js");
}

<?php
	echo "var sliceNumberUUIDOdd = \"" . $sliceNumberUUIDOdd . "\";\n";
?>
wo.postMessage(sliceNumberUUIDOdd); //send the instanceNumbers and UUIDs to the webworker

//when webworker manages to load each UUID into pngToGray, it will postMessage the compressed image data-string back to main script
wo.onmessage = function(event){
	var workerData = event.data.split(','); //CSV again, [0] is instance number, [1] is compressed image data-string
	sliceStream[workerData[0]] = JXG.decompress(workerData[1]); 
	lastSlice = workerData[0];
	document.getElementById("loadstatus").innerHTML = "Loading: " + workerData[0] + "/" + (sliceUUID.length - 1);
};

//create a new webworker to load the image data-strings based on the instanceUUIDs from pngToGray and passes it back to main script
var we;
if(typeof(we) == "undefined") {
    we = new Worker("bgLoadEven.js");
}

<?php
	echo "var sliceNumberUUIDEven = \"" . $sliceNumberUUIDEven . "\";\n";
?>
we.postMessage(sliceNumberUUIDEven); //send the instanceNumbers and UUIDs to the webworker

//when webworker manages to load each UUID into pngToGray, it will postMessage the compressed image data-string back to main script
we.onmessage = function(event){
	var workerData = event.data.split(','); //CSV again, [0] is instance number, [1] is compressed image data-string
	sliceStream[workerData[0]] = JXG.decompress(workerData[1]); 
	lastSlice = workerData[0];
	document.getElementById("loadstatus").innerHTML = "Loading: " + workerData[0] + "/" + (sliceUUID.length - 1);
};

//Odd + Even
//create a new webworker to load the image data-strings based on the instanceUUIDs from pngToGray and passes it back to main script
var wo2;
if(typeof(wo2) == "undefined") {
    wo2 = new Worker("bgLoadOddTwo.js");
}

<?php
	echo "var sliceNumberUUIDOddTwo = \"" . $sliceNumberUUIDOddTwo . "\";\n";
?>
wo2.postMessage(sliceNumberUUIDOddTwo); //send the instanceNumbers and UUIDs to the webworker

//when webworker manages to load each UUID into pngToGray, it will postMessage the compressed image data-string back to main script
wo2.onmessage = function(event){
	var workerData = event.data.split(','); //CSV again, [0] is instance number, [1] is compressed image data-string
	sliceStream[workerData[0]] = JXG.decompress(workerData[1]); 
	lastSlice = workerData[0];
	document.getElementById("loadstatus").innerHTML = "Loading: " + workerData[0] + "/" + (sliceUUID.length - 1);
};

//create a new webworker to load the image data-strings based on the instanceUUIDs from pngToGray and passes it back to main script
var we2;
if(typeof(we2) == "undefined") {
    we2 = new Worker("bgLoadEvenTwo.js");
}

<?php
	echo "var sliceNumberUUIDEvenTwo = \"" . $sliceNumberUUIDEvenTwo . "\";\n";
?>
we2.postMessage(sliceNumberUUIDEvenTwo); //send the instanceNumbers and UUIDs to the webworker

//when webworker manages to load each UUID into pngToGray, it will postMessage the compressed image data-string back to main script
we2.onmessage = function(event){
	var workerData = event.data.split(','); //CSV again, [0] is instance number, [1] is compressed image data-string
	sliceStream[workerData[0]] = JXG.decompress(workerData[1]); 
	lastSlice = workerData[0];
	document.getElementById("loadstatus").innerHTML = "Loading: " + workerData[0] + "/" + (sliceUUID.length - 1);
};


/* //Original
//create a new webworker to load the image data-strings based on the instanceUUIDs from pngToGray and passes it back to main script
var w;
if(typeof(w) == "undefined") {
    w = new Worker("bgLoad.js");
}

<?php
//	echo "var sliceNumberUUID = \"" . $sliceNumberUUID . "\";\n";
?>
w.postMessage(sliceNumberUUID); //send the instanceNumbers and UUIDs to the webworker

//when webworker manages to load each UUID into pngToGray, it will postMessage the compressed image data-string back to main script
w.onmessage = function(event){
	var workerData = event.data.split(','); //CSV again, [0] is instance number, [1] is compressed image data-string
	sliceStream[workerData[0]] = JXG.decompress(workerData[1]); 
	lastSlice = workerData[0];
	document.getElementById("loadstatus").innerHTML = "Loading: " + workerData[0] + "/" + (sliceUUID.length - 1);
};
*/

//mouse events on the canvas
canvas.addEventListener('mousedown', mouseDownListener, false);
canvas.addEventListener('mousewheel',function(evt){
    mouseWheel(evt);
    return false;
}, false);
canvas.addEventListener('contextmenu', blockContextMenu);
//document.addEventListener('contextmenu', blockContextMenu); //dammit, right click fails to open context when clicked outside. Need to be fixed.

var initialLeftX;
var initialLeftY;
var whichButton;

//mousewheel scrolls the stack
function mouseWheel(evt) {
		if (evt.wheelDelta > 0) { 
			adjust(1,0,0,0,0,0);
			} else {
			adjust(-1,0,0,0,0,0);
			}
		}

function mouseDownListener(evt) {
		whichButton =  evt.which;
		//getting mouse position correctly, being mindful of resizing that may have occurred in the browser:
		var bRect = canvas.getBoundingClientRect();
		mouseX = (evt.clientX - bRect.left)*(canvas.width/bRect.width);
		mouseY = (evt.clientY - bRect.top)*(canvas.height/bRect.height);
		initialLeftX = mouseX;
		initialLeftY = mouseY;

		window.addEventListener("mousemove", mouseMoveListener, false);
		canvas.removeEventListener("mousedown", mouseDownListener, false);
		window.addEventListener("mouseup", mouseUpListener, false);
		}
		
function mouseMoveListener(evt) {
		var posX;
		var posY;
		//getting mouse position correctly 
		var bRect = canvas.getBoundingClientRect();
		mouseX = (evt.clientX - bRect.left)*(canvas.width/bRect.width);
		mouseY = (evt.clientY - bRect.top)*(canvas.height/bRect.height);
		var leftXadj = mouseX - initialLeftX;
		var leftYadj = mouseY - initialLeftY;

		if (evt.which == 1) { //left button - panning
			adjust(0,0,0,leftXadj,leftYadj,0);
			}
//		if (evt.which == 2) { //middle button - zooming not working yet
//			adjust(0,0,0,0,0,(leftXadj + leftYadj) / 50);
//			}
		if (evt.which == 3) { //right button - windowing
			adjust(0,leftYadj,leftXadj,0,0,0);
			}
		}		

function mouseUpListener(evt) {
		canvas.addEventListener("mousedown", mouseDownListener, false);
		window.removeEventListener("mouseup", mouseUpListener, false);
		window.removeEventListener("mousemove", mouseMoveListener, false);
		}
//disables right click context menu
function blockContextMenu(evt) {
	evt.preventDefault();
	}
</script>

<hr size=1 width=600>
<font color=#cccccc>mcdcm powered by Orthanc</font>
</font>
</center>

