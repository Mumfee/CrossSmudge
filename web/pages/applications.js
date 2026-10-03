function formatSize(bytes) {
  if (bytes >= 1048576) return (bytes / 1048576).toFixed(1) + ' MB';
  if (bytes >= 1024) return (bytes / 1024).toFixed(0) + ' KB';
  return bytes + ' B';
}

function showStatus(message, isError) {
  const status = document.getElementById('status');
  status.className = isError ? 'status-err' : 'status-ok';
  status.textContent = message;
}

function clearStatus() {
  const status = document.getElementById('status');
  status.style.display = 'none';
  status.className = '';
  status.textContent = '';
}

function setProgress(percent, text) {
  const wrap = document.getElementById('uploadProgress');
  const fill = document.getElementById('progressFill');
  const label = document.getElementById('progressLabel');
  if (percent === null) {
    wrap.style.display = 'none';
    fill.style.width = '0%';
    label.textContent = '';
  } else {
    wrap.style.display = 'block';
    fill.style.width = Math.min(100, Math.max(0, percent)) + '%';
    label.textContent = text || '';
  }
}

// Render 128-byte 1-bit bitmap onto 32x32 canvas
function renderIcon(hexData, canvas) {
  const ctx = canvas.getContext('2d');
  const imgData = ctx.createImageData(32, 32);
  const bytes = [];
  for (let i = 0; i < hexData.length; i += 2) {
    bytes.push(parseInt(hexData.substr(i, 2), 16));
  }
  for (let targetY = 0; targetY < 32; targetY++) {
    for (let targetX = 0; targetX < 32; targetX++) {
      // In CrossPoint firmware, icon bitmaps are stored pre-rotated 90 deg CCW for fast hardware e-ink blitting.
      // Rotate 90 deg clockwise to render upright on the HTML canvas:
      // rawX = targetY, rawY = 31 - targetX
      const rawX = targetY;
      const rawY = 31 - targetX;
      const byteIdx = (rawY * 4) + Math.floor(rawX / 8);
      const bitIdx = 7 - (rawX % 8);
      const b = bytes[byteIdx] !== undefined ? bytes[byteIdx] : 0xFF;
      // In CrossPoint 1-bit icons, 0 bit is drawn pixel (black), 1 bit is background (white/transparent)
      const isDrawn = ((b >> bitIdx) & 1) === 0;
      const pixelIdx = (targetY * 32 + targetX) * 4;
      if (isDrawn) {
        imgData.data[pixelIdx] = 20;     // R
        imgData.data[pixelIdx + 1] = 20; // G
        imgData.data[pixelIdx + 2] = 20; // B
        imgData.data[pixelIdx + 3] = 255;// A
      } else {
        imgData.data[pixelIdx] = 255;
        imgData.data[pixelIdx + 1] = 255;
        imgData.data[pixelIdx + 2] = 255;
        imgData.data[pixelIdx + 3] = 0;
      }
    }
  }
  ctx.putImageData(imgData, 0, 0);
}

async function loadApplications() {
  const el = document.getElementById('appsList');
  try {
    const res = await fetch('/api/applications');
    const data = await res.json();
    el.replaceChildren();

    if (!data.apps || data.apps.length === 0) {
      const p = document.createElement('p');
      p.className = 'empty';
      p.textContent = 'No applications installed in /.crosssmudge/applications.';
      el.appendChild(p);
      return;
    }

    for (const app of data.apps) {
      const row = document.createElement('div');
      row.className = 'app-item';

      const left = document.createElement('div');
      left.className = 'app-left';

      const canvas = document.createElement('canvas');
      canvas.className = 'app-icon';
      canvas.width = 32;
      canvas.height = 32;
      if (app.icon && app.icon.length >= 256) {
        renderIcon(app.icon, canvas);
      } else {
        // Default blank placeholder
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = '#f0f0f0';
        ctx.fillRect(0, 0, 32, 32);
      }
      left.appendChild(canvas);

      const details = document.createElement('div');
      details.className = 'app-details';

      const titleLine = document.createElement('div');
      titleLine.className = 'app-title-line';

      const h3 = document.createElement('h3');
      h3.className = 'app-name';
      h3.textContent = app.name || app.id;
      titleLine.appendChild(h3);

      if (app.version) {
        const badge = document.createElement('span');
        badge.className = 'app-badge';
        badge.textContent = 'v' + app.version;
        titleLine.appendChild(badge);
      }

      if (app.author) {
        const author = document.createElement('span');
        author.className = 'app-author';
        author.textContent = 'by ' + app.author;
        titleLine.appendChild(author);
      }
      details.appendChild(titleLine);

      if (app.description) {
        const desc = document.createElement('p');
        desc.className = 'app-desc';
        desc.textContent = app.description;
        details.appendChild(desc);
      }

      const meta = document.createElement('span');
      meta.className = 'app-meta';
      meta.textContent = 'ID: ' + app.id + ' · Size: ' + formatSize(app.size || 0);
      details.appendChild(meta);

      left.appendChild(details);
      row.appendChild(left);

      // Action buttons
      const actions = document.createElement('div');
      actions.className = 'app-actions';

      const btnDownload = document.createElement('button');
      btnDownload.className = 'btn btn-secondary';
      btnDownload.textContent = 'Download ZIP';
      btnDownload.addEventListener('click', () => downloadAppZip(app.id, app.name));
      actions.appendChild(btnDownload);

      const btnMove = document.createElement('button');
      btnMove.className = 'btn btn-secondary';
      btnMove.textContent = 'Move to PC';
      btnMove.addEventListener('click', () => moveAppToPc(app.id, app.name));
      actions.appendChild(btnMove);

      const btnDelete = document.createElement('button');
      btnDelete.className = 'btn btn-danger';
      btnDelete.textContent = 'Delete';
      btnDelete.addEventListener('click', () => deleteApp(app.id, app.name));
      actions.appendChild(btnDelete);

      row.appendChild(actions);
      el.appendChild(row);
    }
  } catch (err) {
    el.replaceChildren();
    const p = document.createElement('p');
    p.className = 'empty';
    p.textContent = 'Failed to load applications: ' + err.message;
    el.appendChild(p);
  }
}

async function fetchAppFilesZip(appId, appName, onProgress) {
  if (typeof JSZip === 'undefined') {
    throw new Error('JSZip library is not loaded');
  }
  const res = await fetch('/api/applications/files?id=' + encodeURIComponent(appId));
  if (!res.ok) throw new Error('Failed to retrieve file list for ' + appId);
  const data = await res.json();
  const files = data.files || [];
  if (files.length === 0) throw new Error('No files found in application folder');

  const zip = new JSZip();
  const folder = zip.folder(appId);

  let done = 0;
  for (const f of files) {
    if (onProgress) onProgress(done, files.length, f.name);
    const fileRes = await fetch('/api/applications/download?id=' + encodeURIComponent(appId) + '&file=' + encodeURIComponent(f.name));
    if (!fileRes.ok) throw new Error('Failed to download ' + f.name);
    const blob = await fileRes.blob();
    folder.file(f.name, blob);
    done++;
  }

  if (onProgress) onProgress(done, files.length, 'Generating ZIP archive...');
  return await zip.generateAsync({ type: 'blob' });
}

function triggerBlobDownload(blob, filename) {
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  setTimeout(() => URL.revokeObjectURL(url), 2000);
}

async function downloadAppZip(appId, appName) {
  clearStatus();
  setProgress(10, 'Preparing download for ' + (appName || appId) + '...');
  try {
    const zipBlob = await fetchAppFilesZip(appId, appName, (done, total, current) => {
      const pct = Math.round((done / Math.max(1, total)) * 100);
      setProgress(pct, 'Downloading ' + done + '/' + total + ': ' + current);
    });
    triggerBlobDownload(zipBlob, appId + '.zip');
    setProgress(null);
    showStatus('Application package "' + (appName || appId) + '" downloaded successfully.', false);
  } catch (err) {
    setProgress(null);
    showStatus('Download failed: ' + err.message, true);
  }
}

async function moveAppToPc(appId, appName) {
  clearStatus();
  setProgress(10, 'Preparing move for ' + (appName || appId) + '...');
  try {
    const zipBlob = await fetchAppFilesZip(appId, appName, (done, total, current) => {
      const pct = Math.round((done / Math.max(1, total)) * 100);
      setProgress(pct, 'Downloading ' + done + '/' + total + ': ' + current);
    });
    triggerBlobDownload(zipBlob, appId + '.zip');
    setProgress(null);

    const shouldDelete = confirm(
      'Application "' + (appName || appId) + '" has been downloaded to your computer as ' + appId + '.zip.\n\n' +
      'Do you want to delete it from the device now to complete moving it to your PC?'
    );

    if (shouldDelete) {
      await performDeleteApp(appId, appName);
    } else {
      showStatus('Downloaded "' + (appName || appId) + '". Kept on device.', false);
    }
  } catch (err) {
    setProgress(null);
    showStatus('Move failed: ' + err.message, true);
  }
}

async function performDeleteApp(appId, appName) {
  clearStatus();
  showStatus('Deleting "' + (appName || appId) + '" from device...', false);
  try {
    const res = await fetch('/api/applications/delete', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ id: appId })
    });
    const data = await res.json();
    if (res.ok && data.ok) {
      showStatus('Deleted application "' + (appName || appId) + '" from device.', false);
      await loadApplications();
    } else {
      showStatus('Failed to delete: ' + (data.error || 'Server error'), true);
    }
  } catch (err) {
    showStatus('Delete error: ' + err.message, true);
  }
}

async function deleteApp(appId, appName) {
  if (!confirm('Are you sure you want to delete application "' + (appName || appId) + '" from the device?')) {
    return;
  }
  await performDeleteApp(appId, appName);
}

// --- Upload Pipeline (Zip or Folder) ---

async function uploadFileList(appId, fileList) {
  clearStatus();
  let uploaded = 0;
  for (const item of fileList) {
    const pct = Math.round((uploaded / fileList.length) * 100);
    setProgress(pct, 'Uploading ' + (uploaded + 1) + '/' + fileList.length + ': ' + item.path);

    const formData = new FormData();
    formData.append('file', item.blob, item.name);

    const url = '/api/applications/upload?app=' + encodeURIComponent(appId) +
                '&path=' + encodeURIComponent(item.path);

    const res = await fetch(url, { method: 'POST', body: formData });
    if (!res.ok) {
      const errText = await res.text();
      throw new Error('Failed to upload ' + item.path + ' (' + errText + ')');
    }
    uploaded++;
  }
  setProgress(100, 'Upload complete!');
  setTimeout(() => setProgress(null), 1000);
  showStatus('Successfully installed application "' + appId + '" (' + uploaded + ' files).', false);
  await loadApplications();
}

async function handleZipFile(zipFile) {
  clearStatus();
  setProgress(10, 'Unpacking ' + zipFile.name + '...');
  try {
    if (typeof JSZip === 'undefined') {
      throw new Error('JSZip library is not available');
    }
    const zip = await JSZip.loadAsync(zipFile);

    // Look for manifest.json
    let manifestEntry = null;
    let rootPrefix = '';

    zip.forEach((relPath, entry) => {
      if (!entry.dir && (relPath === 'manifest.json' || relPath.endsWith('/manifest.json'))) {
        if (!manifestEntry || relPath.split('/').length < manifestEntry.split('/').length) {
          manifestEntry = relPath;
        }
      }
    });

    let appId = '';
    if (manifestEntry) {
      const lastSlash = manifestEntry.lastIndexOf('/');
      if (lastSlash >= 0) {
        rootPrefix = manifestEntry.substring(0, lastSlash + 1);
      }
      try {
        const manifestText = await zip.files[manifestEntry].async('text');
        const doc = JSON.parse(manifestText);
        if (doc.id) appId = doc.id.replace(/[^A-Za-z0-9_-]/g, '_');
      } catch (e) {
        console.warn('Could not parse manifest.json', e);
      }
    }

    if (!appId) {
      if (rootPrefix) {
        appId = rootPrefix.replace(/\/$/, '').replace(/[^A-Za-z0-9_-]/g, '_');
      } else {
        appId = zipFile.name.replace(/\.zip$/i, '').replace(/[^A-Za-z0-9_-]/g, '_');
      }
    }

    if (!appId) appId = 'app_' + Date.now();

    // Collect files under rootPrefix
    const items = [];
    const entries = [];
    zip.forEach((relPath, entry) => {
      if (!entry.dir) {
        if (!rootPrefix || relPath.startsWith(rootPrefix)) {
          entries.push({ relPath, entry });
        }
      }
    });

    if (entries.length === 0) {
      throw new Error('No files found inside zip archive.');
    }

    for (const { relPath, entry } of entries) {
      const cleanRel = rootPrefix ? relPath.substring(rootPrefix.length) : relPath;
      if (!cleanRel) continue;
      const blob = await entry.async('blob');
      const filename = cleanRel.substring(cleanRel.lastIndexOf('/') + 1);
      items.push({
        path: cleanRel,
        name: filename,
        blob: blob
      });
    }

    await uploadFileList(appId, items);
  } catch (err) {
    setProgress(null);
    showStatus('ZIP installation error: ' + err.message, true);
  }
}

async function handleDirectoryFiles(files) {
  clearStatus();
  if (!files || files.length === 0) return;

  // Determine top folder / app name
  let firstRel = files[0].webkitRelativePath || files[0].name;
  let topFolder = '';
  if (firstRel.includes('/')) {
    topFolder = firstRel.split('/')[0];
  } else {
    topFolder = 'app_' + Date.now();
  }

  // Check for manifest.json to derive id
  let appId = topFolder.replace(/[^A-Za-z0-9_-]/g, '_');
  for (const file of files) {
    const rel = file.webkitRelativePath || file.name;
    if (rel === 'manifest.json' || rel.endsWith('/manifest.json')) {
      try {
        const text = await file.text();
        const doc = JSON.parse(text);
        if (doc.id) appId = doc.id.replace(/[^A-Za-z0-9_-]/g, '_');
      } catch (e) {}
      break;
    }
  }

  const items = [];
  for (const file of files) {
    const rel = file.webkitRelativePath || file.name;
    let cleanRel = rel;
    if (topFolder && cleanRel.startsWith(topFolder + '/')) {
      cleanRel = cleanRel.substring(topFolder.length + 1);
    }
    items.push({
      path: cleanRel,
      name: file.name,
      blob: file
    });
  }

  try {
    await uploadFileList(appId, items);
  } catch (err) {
    setProgress(null);
    showStatus('Folder installation error: ' + err.message, true);
  }
}

// Setup Event Listeners
document.getElementById('btnRefresh').addEventListener('click', loadApplications);

const btnSelectFolder = document.getElementById('btnSelectFolder');
const btnSelectZip = document.getElementById('btnSelectZip');
const inputFolder = document.getElementById('inputFolder');
const inputZip = document.getElementById('inputZip');
const dropZone = document.getElementById('dropZone');

btnSelectFolder.addEventListener('click', (e) => {
  e.stopPropagation();
  inputFolder.click();
});

btnSelectZip.addEventListener('click', (e) => {
  e.stopPropagation();
  inputZip.click();
});

inputFolder.addEventListener('change', () => {
  if (inputFolder.files.length > 0) {
    handleDirectoryFiles(Array.from(inputFolder.files));
    inputFolder.value = '';
  }
});

inputZip.addEventListener('change', () => {
  if (inputZip.files.length > 0) {
    handleZipFile(inputZip.files[0]);
    inputZip.value = '';
  }
});

dropZone.addEventListener('dragover', (e) => {
  e.preventDefault();
  dropZone.classList.add('dragover');
});

dropZone.addEventListener('dragleave', () => {
  dropZone.classList.remove('dragover');
});

dropZone.addEventListener('drop', async (e) => {
  e.preventDefault();
  dropZone.classList.remove('dragover');

  const items = e.dataTransfer.items;
  if (!items || items.length === 0) {
    if (e.dataTransfer.files.length > 0) {
      const file = e.dataTransfer.files[0];
      if (file.name.toLowerCase().endsWith('.zip')) {
        handleZipFile(file);
      } else {
        handleDirectoryFiles(Array.from(e.dataTransfer.files));
      }
    }
    return;
  }

  // Check first item
  const item = items[0];
  const entry = item.webkitGetAsEntry ? item.webkitGetAsEntry() : null;

  if (entry && entry.isDirectory) {
    // Read directory recursively
    const files = [];
    async function readEntry(ent, path) {
      if (ent.isFile) {
        const file = await new Promise((resolve, reject) => ent.file(resolve, reject));
        file.customPath = (path ? path + '/' : '') + file.name;
        files.push(file);
      } else if (ent.isDirectory) {
        const reader = ent.createReader();
        const subEntries = await new Promise((resolve, reject) => reader.readEntries(resolve, reject));
        for (const sub of subEntries) {
          await readEntry(sub, (path ? path + '/' : '') + ent.name);
        }
      }
    }
    await readEntry(entry, '');
    for (const f of files) {
      f.webkitRelativePath = f.customPath;
    }
    handleDirectoryFiles(files);
  } else {
    const file = item.getAsFile();
    if (file) {
      if (file.name.toLowerCase().endsWith('.zip')) {
        handleZipFile(file);
      } else {
        handleDirectoryFiles([file]);
      }
    }
  }
});

loadApplications();
