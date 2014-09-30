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
//    return(JXG.decompress(allText));
return(allText);
}

var sliceNumberUUID = [];
self.addEventListener('message', function(e) {
	sliceNumberUUID = e.data.split(','); //retrieves entire list of instances from main script in CSV
	var s;
	var sliceUUID = []; //to hold instanceNumber
	var sliceStream = []; //to hold compressed image data-string
	for (s = 0; sliceNumberUUID[s] != null; s = s + 2) {
		sliceUUID[parseInt(sliceNumberUUID[s])] = sliceNumberUUID[s+1]; //sliceNumberUUID can be "1 ", so needs ParseInt
		}
	for (s = 1; sliceUUID[s] != null; s = s + 4) { //instanceNumber[1] already loaded by main script.
		sliceStream[s] = (getGrayFromPNG('pngToGray.php?instance=' + sliceUUID[s]));
		postMessage(s + "," + sliceStream[s]); //returns InstanceNumber "," compressed image data-string
		}
}, false);


