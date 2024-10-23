 /**
 * @file webpage.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief Web file server page
 * @version 0.1.8_Alpha
 * @date 2024-10
 */


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">

<head>

  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">

  <style>
      header {
            font-family: "Lucida Console", "Courier New", monospace;
            font-weight: bold;
            font-size: 30px;
            color: white;
            background-color:  #ffffff;
          }
      .p3 {
            font-family: "Lucida Console", "Courier New", monospace;
            font-size: 12px;
            color: white;
          }
      .p2 {
            font-family: "Lucida Console", "Courier New", monospace;
            font-size: 10px;
            color: white;
          }
      .button {
                  background-color:  #5d6d7e;
                  border: none;
                  color: white;
                  padding: 5px 25px;
                  text-align: center;
                  text-decoration: none;
                  display: inline-block;
                  font-size: 12px;
                  opacity: 0.6;
                  transition: 0.3s;
                  margin: 4px 2px;
                  cursor: pointer;
              }
      .button:hover {opacity: 1}

      a:link {
                font-weight: bold;
                font-style: italic;
                text-decoration: none;
                color: white;
             }

      a:hover {
                color: blue;
                background-color: transparent;
                text-decoration: none;
              }

      a:visited {
                color: white;
                background-color: transparent;
                text-decoration: none;
                }

      th, td {
                padding: 4px;
                text-align: left;
                border-bottom: 1px solid #ddd;
              }

      ti     {
                padding: 4px;
                text-align: left;
             }
      td {
            font-family: "Lucida Console", "Courier New", monospace;
            font-size: 10px;
            height: 25px;
            vertical-align: middle;
          }

      #drag {
            width: 350px;
            height: 70px;
            padding: 10px;
            background-color:  black;
            border: 1px solid #aaaaaa;
            text-align: center;
          }    
  </style>

</head>

<body onload="refresh()">

    <style>
      body {
             background-color: black;
           }
    </style>

  <img src="logo">

  <p class="p3">Firmware: %FIRMWARE%</p>
  <p class="p3">Type: %TYPEFS% | Size: <span id="size">%TOTALFS%</span> | Used: <span id="used">%USEDFS%</span> | Free: <span id="free">%FREEFS%</span></p>

  <p>
  <button class="button" onclick="rebootButton()"><img src="reb"> Reboot</button>
  <button class="button" onclick="listFilesButton()"><img src="list"> List Files</button>
  <button class="button" onclick="uploadButton()"><img src="up"> Upload File</button>
  </p>

  <p class="p3" id="status"></p>
  <p class="p3" id="detailsheader"></p>
  <p class="p3" id="details"></p>

<script>

function _(el)
{
  return document.getElementById(el);
}

function loadPage(page) 
{
  fetch('/listfiles?page=' + page) 
    .then(response => response.text())
    .then(data => {
      document.getElementById("details").innerHTML = data; 
    });
}

function refresh()
{
  xhr = new XMLHttpRequest();
  xhr.open("GET", "/listfiles", false);
  xhr.send();
  _("status").innerHTML = sessionStorage.getItem("msgStatus");
  sessionStorage.removeItem("msgStatus");
  _("detailsheader").innerHTML = "<h3>Files<h3>";
  _("details").innerHTML = xhr.responseText;
}

function rebootButton()
{
  _("status").innerHTML = "Invoking Reboot ...";
  xhr = new XMLHttpRequest();
  xhr.open("GET", "/reboot", true);
  xhr.send();
  window.open("/reboot","_self");
}

function listFilesButton()
{
  xhr = new XMLHttpRequest();
  xhr.open("GET", "/listfiles", false);
  xhr.send();
  _("detailsheader").innerHTML = "<h3>Files<h3>";
  _("details").innerHTML = xhr.responseText;
}

function downloadDeleteButton(filename, action) 
{
  var urltocall = "/file?name=/" + filename + "&action=" + action;
  xhr = new XMLHttpRequest();
  if (action == "delete")
  {
    xhr.open("GET", urltocall, false);
    xhr.send();
    sessionStorage.setItem("msgStatus",xhr.responseText);
    _("details").innerHTML = xhr.responseText;
    document.location.reload(true);   
  }
  if (action == "download") 
  {
    _("status").innerHTML = "";
    window.open(urltocall,"_blank");
  }
}

function changeDirectory(directory)
{
  var encodedDirectory = encodeURIComponent(directory);
  var urltocall = "/changedirectory?dir=/" + encodedDirectory;
  xhr = new XMLHttpRequest();
  xhr.open("GET", urltocall, false);
  xhr.send();
  _("status").innerHTML = xhr.responseText;
  xhr.open("GET", "/listfiles", false);
  xhr.send();
  _("detailsheader").innerHTML = "<h3>Files<h3>";
  _("details").innerHTML = xhr.responseText;
}

function dragged(e) 
{  
  var z = _("drag");
  z.style.backgroundColor = "#5d6d7e";
  z.style.opacity = "0.6";
  e.stopPropagation();
  e.preventDefault();
}

function dropped(e) 
{
  dragged(e);
  var fls = e.dataTransfer.files;
  var formData = new FormData();

  for (var i = 0; i < fls.length; i++) 
  {
    formData.append("file" + i, fls[i]); 
  }
  var z = document.getElementById("drag");
  z.style.backgroundColor = "white";

  var fileNames = "";
  for (var i = 0; i < fls.length; i++) 
  {
    fileNames += fls[i].name + (i < fls.length - 1 ? ", " : ""); 
  }
  z.textContent = fileNames;

  var xhr = new XMLHttpRequest();
  xhr.upload.addEventListener("progress", progressHandler, false);
  xhr.addEventListener("load", completeHandler, false); 
  xhr.addEventListener("error", errorHandler, false);
  xhr.addEventListener("abort", abortHandler, false);
  xhr.open("POST", "/");
  xhr.send(formData);   
}

function uploadButton() 
{
  _("detailsheader").innerHTML = "<h3>Upload File(s)</h3>"
  _("status").innerHTML = "";

  var uploadform =
  "<form id=\"upload_form\" enctype=\"multipart/form-data\" method=\"post\">" +
  "<div id=\"drag\"><h3 id=\"filetext\">Drag to upload file</h3></div>" + 
  "<progress id=\"progressBar\" value=\"0\" max=\"100\" style=\"width:372px;\"></progress>" +
  "<h3 id=\"status\"></h3>" +
  "<p id=\"loaded_n_total\"></p>" +
  "</form>";
  
  _("details").innerHTML = uploadform;

  var z = _("drag");
  z.addEventListener("dragenter", dragged, false);
  z.addEventListener("dragover", dragged, false);
  z.addEventListener("drop", dropped, false);
}

function progressHandler(event) 
{
  var percent = (event.loaded / event.total) * 100;
  _("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes of " + event.total; // event.total doesn't show accurate total file size
 
  //_("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes";
   _("progressBar").value = Math.round(percent);
  _("status").innerHTML = Math.round(percent) + "% uploaded... please wait";
  if (percent >= 100)
  {
    _("status").innerHTML = "Please wait, writing file to filesystem";
  }
}


function completeHandler(event) 
{
  _("status").innerHTML = "Upload Complete";
  _("progressBar").value = 0;
  xhr = new XMLHttpRequest();
  xhr.open("GET", "/listfiles", false);
  xhr.send();
  sessionStorage.setItem("msgStatus","File Uploaded");
  _("detailsheader").innerHTML = "<h3>Files<h3>";
  _("details").innerHTML = xhr.responseText;
  document.location.reload(true);
}

function errorHandler(event) 
{
  _("status").innerHTML = "Upload Failed";
}

function abortHandler(event) 
{
  _("status").innerHTML = "inUpload Aborted";
}
</script>

</body>
</html>
)rawliteral";



// reboot.html base upon https://gist.github.com/Joel-James/62d98e8cb3a1b6b05102
const char reboot_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
</head>
<body>
<h3>
  Rebooting, returning to main page in <span id="countdown">30</span> seconds
</h3>
<script type="text/javascript">
  var seconds = 20;
  function countdown() {
    seconds = seconds - 1;
    if (seconds < 0) {
      window.location = "/";
    } else {
      document.getElementById("countdown").innerHTML = seconds;
      window.setTimeout("countdown()", 1000);
    }
  }
  countdown();
</script>
</body>
</html>
)rawliteral";
