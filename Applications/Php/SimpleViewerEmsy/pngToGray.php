<?php
ini_set('memory_limit', '2048M'); //needed if we want to use un-resized 4000px x 4000px files
error_reporting(E_ALL);
ini_set('display_errors', 'On');

$orthancAddress = "http://localhost:8042";

$pngFile = bin2hex(file_get_contents($orthancAddress . "/instances/" . $_GET['instance'] . "/image-uint16")); //returns in PNG format

//decodes the PNG file. Wrote this myself because I couldn't find a 12-bit decoder...
$posIHDR = strpos($pngFile, "49484452");
$posIEND = strpos($pngFile, "49454e44");
$width = hexdec(substr($pngFile, $posIHDR + 8, 8));
$height = hexdec(substr($pngFile, $posIHDR + 16, 8));

//reduce the heights / widths to less than 1000. Correlates with viewer.php
//NEEDS imagemagick installed. Can't use GD2 because GD2 doesn't work with 12-bit grayscale!
//if no imagemagick, can try to comment out the lines here and in viewer.php, but hope that PHP has enough memory to process the large image.
if ($width > 2000 || $height > 2000) {
	file_put_contents('image.png', hex2bin($pngFile));
	$width = $width / 4;
	$height = $height / 4;
	exec('"c:\imagick\convert.exe" "image.png" "-scale" "25%" "image.png"');
	$pngFile = bin2hex(file_get_contents("image.png"));
	}
if ($width > 1000 || $height > 1000) {
	file_put_contents('image.png', hex2bin($pngFile));
	$width = $width / 2;
	$height = $height / 2;
	exec('"c:\imagick\convert.exe" "image.png" "-scale" "50%" "image.png"');
	$pngFile = bin2hex(file_get_contents("image.png"));
	}

$lastPos = 0;
$data = "";
while (($lastPos = strpos($pngFile, "49444154", $lastPos))!== false) { //get all the IDAT locations
	$chunkLength = hexdec(substr($pngFile, $lastPos - 8, 8)); //get the IDAT chunk length
	$data = $data . substr($pngFile, $lastPos + 8, $chunkLength * 2); //concatenate the chunks
	$lastPos = $lastPos + 8;
}

$hex = bin2hex(gzuncompress(hex2bin($data))); //uncompress the concatenated chunk. I can't find a gz-uncompress in Javascript that works with the PNG files that Orthanc generates, so I'm doing the unfiltering server-side in PHP.

$byte[-1] = "00";
$p = 0;
for ($i = 0; $i < $height; $i++) {
	$scanline[$i] = substr($hex, ($i * (($width * 4) + 2)), ($width * 4) + 2); //get each scanline based on width
	for ($j = 0; $j < strlen($scanline[$i]) / 2; $j++) {
		$byte[$j] = substr($scanline[$i], $j * 2, 2); //get the current byte that we're trying to unfilter
		if ($j == 0) { 
			$filter = $byte[0]; 
			$byte[0] = "00";
			$byte[-1] = "00";
			$scan[$i][-1] = 0;
			$scan[$i][0] = 0;
			continue; 
			} // determine filter type
		if ($filter == "00") {
			$scan[$i][$j] = $byte[$j];
			while (hexdec($scan[$i][$j]) > 255) {
				$scan[$i][$j] = dechex(hexdec($scan[$i][$j]) - 256); 
				}
			}
		if ($filter == "01") {
			$scan[$i][$j] = dechex(hexdec($byte[$j]) + hexdec($scan[$i][$j-2])); //$scan[$i][$j] contains unfiltered bytes
			while (hexdec($scan[$i][$j]) > 255) {
				$scan[$i][$j] = dechex(hexdec($scan[$i][$j]) - 256); //modulo 256
				}
			}
		if ($filter == "02") {
			$scan[$i][$j] = dechex(hexdec($byte[$j]) + hexdec($scan[$i-1][$j]));
			while (hexdec($scan[$i][$j]) > 255) {
				$scan[$i][$j] = dechex(hexdec($scan[$i][$j]) - 256);
				}
			}
		if ($filter == "03") {
			$average = (hexdec($scan[$i-1][$j]) + hexdec($scan[$i][$j-2])) / 2;
			$scan[$i][$j] = dechex(hexdec($byte[$j]) + $average);
			while (hexdec($scan[$i][$j]) > 255) {
				$scan[$i][$j] = dechex(hexdec($scan[$i][$j]) - 256);
				}
			}
		if ($filter == "04") { // Paeth
			$paeth = "";
			$l = hexdec($scan[$i][$j-2]);
			$a = hexdec($scan[$i-1][$j]);
			$al = hexdec($scan[$i-1][$j-2]);
			$base = $l + $a - $al;
			if (abs($base - $l) <= abs($base - $a) && abs($base - $l) <= abs($base - $al)) { $paeth = $l; } 
			elseif (abs($base - $a) <= abs($base - $al)) { $paeth = $a; }
			else { $paeth = $al; }
			$scan[$i][$j] = dechex(hexdec($byte[$j]) + $paeth);
			while (hexdec($scan[$i][$j]) > 255) {
				$scan[$i][$j] = dechex(hexdec($scan[$i][$j]) - 256);
				}
			}		
		if (strlen($scan[$i][$j]) < 2) { $scan[$i][$j] = "0" . $scan[$i][$j]; } //turns "0" to "00"
		}	
	}
$p = 0;
for ($i = 0; $i < $height; $i++) {
	for ($j = 1; $j < $width+1; $j++) {
		$pixel[$p] = hexdec($scan[$i][($j*2) - 1] . $scan[$i][($j*2)]); //since a 12-bit pixel fits into 2 bytes, each pixel is placed into a linear array starting from top left to bottom right. Array length will be height * width
		$p++;
		}
	}
$pixels = implode(",", $pixel); //turn $pixel[] into a string

echo base64_encode(gzcompress($pixels,7)); //zip, base-encode and return. 

?>