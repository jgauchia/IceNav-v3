/**
 * @file webpage.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Web file server page
 * @version 0.2.9
 * @date 2026-06
 */

const char index_html[] = R"rawliteral(
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
                        overflow-y: auto;
                        overflow-x: hidden;
                }
        </style>

        <script src="/jszip"></script>
    </head>

    <body onload="refresh()">

        <style>
            body { background-color: black;}
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
        {
            var pollInterval = null;

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

            function startPolling()
            {
                if (pollInterval) return;
                pollInterval = setInterval(function() {
                    fetch('/status')
                        .then(response => response.json())
                        .then(data => {
                            if (data.message) {
                                _("status").innerHTML = data.message;
                            }
                            if (data.refresh) {
                                stopPolling();
                                sessionStorage.removeItem("msgStatus");
                                document.location.reload(true);
                            }
                        })
                        .catch(err => {});
                }, 1000);
            }

            function stopPolling()
            {
                if (pollInterval) {
                    clearInterval(pollInterval);
                    pollInterval = null;
                }
            }

            function downloadDeleteButton(filename, action)
            {
                var urltocall = "";
                xhr = new XMLHttpRequest();
                if (action == "delete")
                {
                        urltocall = "/file?name=/" + encodeURIComponent(filename) + "&action=" + action;
                        xhr.open("GET", urltocall, false);
                        xhr.send();
                        sessionStorage.setItem("msgStatus",xhr.responseText);
                        _("status").innerHTML = "";
                        _("details").innerHTML = "Deleting file: " + filename;
                        document.location.reload(true);
                }
                if (action == "deldir")
                {
                        urltocall = "/file?name=" + encodeURIComponent(filename) + "&action=" + action;
                        xhr.open("GET", urltocall, false);
                        xhr.send();
                        sessionStorage.setItem("msgStatus",xhr.responseText);
                        _("status").innerHTML = "Deleting folder: " + filename + " please wait....";
                        _("details").innerHTML = "";
                        startPolling();
                }
                if (action == "download")
                {
                        urltocall = "/file?name=/" + encodeURIComponent(filename) + "&action=" + action;
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
                e.preventDefault();
                e.stopPropagation();

                var dt = e.dataTransfer;
                var items = dt.items;
                var uploads = [];
                var formData = new FormData();
                var fileNames = "";

                var xhr = new XMLHttpRequest();
                xhr.upload.addEventListener("progress", progressHandler, false);
                xhr.addEventListener("load", completeHandler, false);
                xhr.addEventListener("error", errorHandler, false);
                xhr.addEventListener("abort", abortHandler, false);

                async function processFiles(files)
                {
                    for (const file of files)
                    {
                        formData.append("file[]", file, file.name);
                        fileNames += file.name + ", ";
                    }

                    var z = document.getElementById("drag");
                    z.style.backgroundColor = "black";
                    z.textContent = fileNames;

                    xhr.open("POST", "/");
                    xhr.send(formData);
                }

                async function traverseFileTree(item, path = "")
                {
                    return new Promise((resolve) => {
                        if (item.isFile)
                        {
                            item.file((file) => {
                            const relativePath = path + file.name;
                            const newFile = new File([file], relativePath, { type: file.type });
                            uploads.push(newFile);
                            resolve();
                            });
                        } else if (item.isDirectory)
                        {
                            const dirReader = item.createReader();
                            dirReader.readEntries(async (entries) => {
                            for (const entry of entries) {
                                await traverseFileTree(entry, path + item.name + "/");
                            }
                            resolve();
                            });
                        }
                    });
                }

                async function handleItems(items)
                {
                    const promises = [];
                    for (let i = 0; i < items.length; i++)
                    {
                        const item = items[i].webkitGetAsEntry();
                        if (item)
                        {
                            promises.push(traverseFileTree(item));
                        }
                    }
                    await Promise.all(promises);
                    processFiles(uploads);
                }

                if (items && items.length > 0)
                {
                    handleItems(items);
                }
            }

            function progressHandler(event)
            {
                var percent = (event.loaded / event.total) * 100;
                _("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes of " + event.total;
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

            function downloadFolder(folderName)
            {
                _("status").innerHTML = "Preparing folder download...";

                var bar = document.createElement('progress');
                bar.id = "folderBar";
                bar.value = 0;
                bar.max = 100;
                bar.style = "width:372px;display:block;margin:4px 0;";
                _("status").insertAdjacentElement('afterend', bar);

                fetch('/listfolder?path=' + encodeURIComponent(folderName))
                    .then(response => response.text())
                    .then(async function(data) {
                        var lines = data.split('\n').filter(f => f.trim() !== '');
                        if (lines.length === 0) {
                            _("status").innerHTML = "Folder is empty";
                            bar.remove();
                            return;
                        }
                        var files = lines.map(function(l) {
                            var parts = l.split('|');
                            return { path: parts[0], size: parseInt(parts[1]) || 0 };
                        });
                        var totalBytes = files.reduce(function(acc, f) { return acc + f.size; }, 0);
                        var doneBytes = 0;
                        function fmtBytes(b) {
                            if (b < 1024) return b + " B";
                            if (b < 1048576) return (b / 1024).toFixed(1) + " KB";
                            return (b / 1048576).toFixed(1) + " MB";
                        }
                        var zip = new JSZip();
                        var errors = 0;
                        for (var i = 0; i < files.length; i++) {
                            var filePath = files[i].path;
                            var fileName = filePath.substring(filePath.lastIndexOf('/') + 1);
                            var url = '/file?name=' + encodeURIComponent(filePath) + '&action=download';
                            try {
                                var resp = await fetch(url);
                                if (!resp.ok) throw new Error("HTTP " + resp.status);
                                var reader = resp.body.getReader();
                                var chunks = [];
                                var fileBytes = 0;
                                while (true) {
                                    var result = await reader.read();
                                    if (result.done) break;
                                    chunks.push(result.value);
                                    fileBytes += result.value.byteLength;
                                    _("status").innerHTML = "Fetching " + (i + 1) + "/" + files.length + ": " + fileName + " | " + fmtBytes(doneBytes + fileBytes) + " / " + fmtBytes(totalBytes);
                                    bar.value = Math.round(((doneBytes + fileBytes) / totalBytes) * 90);
                                }
                                var merged = new Uint8Array(fileBytes);
                                var offset = 0;
                                for (var c = 0; c < chunks.length; c++) {
                                    merged.set(chunks[c], offset);
                                    offset += chunks[c].byteLength;
                                }
                                var zipPath = filePath.replace(/^\//, '');
                                zip.file(zipPath, merged);
                                doneBytes += fileBytes;
                            } catch(e) {
                                errors++;
                                _("status").innerHTML = "Error fetching " + fileName + ": " + e.message;
                                await new Promise(r => setTimeout(r, 1500));
                            }
                        }
                        _("status").innerHTML = "Generating ZIP...";
                        bar.value = 95;
                        var content = await zip.generateAsync(
                            { type: "blob", compression: "DEFLATE", compressionOptions: { level: 6 } },
                            function(meta) { bar.value = 95 + Math.round(meta.percent / 20); }
                        );
                        bar.value = 100;
                        var a = document.createElement('a');
                        a.href = URL.createObjectURL(content);
                        a.download = folderName + ".zip";
                        document.body.appendChild(a);
                        a.click();
                        document.body.removeChild(a);
                        URL.revokeObjectURL(a.href);
                        bar.remove();
                        var msg = "Downloaded " + folderName + ".zip (" + (files.length - errors) + "/" + files.length + " files, " + fmtBytes(doneBytes) + ")";
                        if (errors > 0) msg += " - " + errors + " error(s)";
                        _("status").innerHTML = msg;
                    })
                    .catch(function(err) {
                        bar.remove();
                        _("status").innerHTML = "Error: " + err;
                    });
            }
        }
        </script>

    </body>

</html>
)rawliteral";

const char reboot_html[] = R"rawliteral(
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
            function countdown()
            {
                seconds = seconds - 1;
                if (seconds < 0)
                {
                    window.location = "/";
                }
                else
                {
                    document.getElementById("countdown").innerHTML = seconds;
                    window.setTimeout("countdown()", 1000);
                }
            }
            countdown();
        </script>
    </body>

</html>
)rawliteral";
