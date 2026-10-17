import init, { validate_image, split_left, split_right } from './tilesplit_wasm.js';

const dropZone = document.getElementById('dropZone');
const fileInput = document.getElementById('fileInput');
const imageInfo = document.getElementById('imageInfo');
const fileName = document.getElementById('fileName');
const dimensions = document.getElementById('dimensions');
const aspectRatio = document.getElementById('aspectRatio');
const tileSize = document.getElementById('tileSize');
const ultraHdrBadge = document.getElementById('ultraHdrBadge');
const controls = document.getElementById('controls');
const qualitySlider = document.getElementById('qualitySlider');
const qualityValue = document.getElementById('qualityValue');
const splitBtn = document.getElementById('splitBtn');
const errorMsg = document.getElementById('errorMsg');
const previewSection = document.getElementById('previewSection');
const previewLeft = document.getElementById('previewLeft');
const previewRight = document.getElementById('previewRight');
const downloadLeft = document.getElementById('downloadLeft');
const downloadRight = document.getElementById('downloadRight');

let currentFile = null;
let currentData = null;
let leftBlobUrl = null;
let rightBlobUrl = null;

qualitySlider.addEventListener('input', () => {
    qualityValue.textContent = qualitySlider.value;
});

// Drag-and-drop
dropZone.addEventListener('click', () => fileInput.click());
dropZone.addEventListener('dragover', (e) => {
    e.preventDefault();
    dropZone.classList.add('drag-over');
});
dropZone.addEventListener('dragleave', () => {
    dropZone.classList.remove('drag-over');
});
dropZone.addEventListener('drop', (e) => {
    e.preventDefault();
    dropZone.classList.remove('drag-over');
    const file = e.dataTransfer.files[0];
    if (file) handleFile(file);
});
fileInput.addEventListener('change', () => {
    const file = fileInput.files[0];
    if (file) handleFile(file);
});

function showError(msg) {
    errorMsg.textContent = msg;
    errorMsg.hidden = false;
}

function clearError() {
    errorMsg.hidden = true;
}

function clearPreviews() {
    previewSection.hidden = true;
    if (leftBlobUrl) URL.revokeObjectURL(leftBlobUrl);
    if (rightBlobUrl) URL.revokeObjectURL(rightBlobUrl);
    leftBlobUrl = null;
    rightBlobUrl = null;
}

function handleFile(file) {
    clearError();
    clearPreviews();

    if (!file.type.match(/image\/jpeg/) && !file.name.match(/\.(jpg|jpeg)$/i)) {
        showError('Please select a JPEG image.');
        return;
    }

    currentFile = file;
    const reader = new FileReader();
    reader.onload = (e) => {
        currentData = new Uint8Array(e.target.result);
        validateAndShow();
    };
    reader.readAsArrayBuffer(file);
}

function validateAndShow() {
    clearError();

    try {
        const info = validate_image(currentData);
        fileName.textContent = currentFile.name;
        dimensions.textContent = `${info.width} x ${info.height}`;
        aspectRatio.textContent = info.aspect;
        tileSize.textContent = `Tile: ${info.tileWidth} x ${info.tileHeight}`;
        ultraHdrBadge.hidden = !info.isUltraHdr;
        imageInfo.hidden = false;
        controls.hidden = false;
    } catch (e) {
        imageInfo.hidden = true;
        controls.hidden = true;
        showError(String(e));
    }
}

splitBtn.addEventListener('click', () => {
    if (!currentData) return;

    clearError();
    clearPreviews();
    splitBtn.disabled = true;
    splitBtn.textContent = 'Splitting\u2026';

    // Defer to allow UI update
    setTimeout(() => {
        try {
            const quality = parseInt(qualitySlider.value, 10);
            const leftBytes = split_left(currentData, quality);
            const rightBytes = split_right(currentData, quality);

            const leftBlob = new Blob([leftBytes], { type: 'image/jpeg' });
            const rightBlob = new Blob([rightBytes], { type: 'image/jpeg' });

            leftBlobUrl = URL.createObjectURL(leftBlob);
            rightBlobUrl = URL.createObjectURL(rightBlob);

            previewLeft.src = leftBlobUrl;
            previewRight.src = rightBlobUrl;

            const stem = currentFile.name.replace(/\.(jpg|jpeg)$/i, '');
            downloadLeft.href = leftBlobUrl;
            downloadLeft.download = `${stem}-left.jpg`;
            downloadRight.href = rightBlobUrl;
            downloadRight.download = `${stem}-right.jpg`;

            previewSection.hidden = false;
        } catch (e) {
            showError(String(e));
        } finally {
            splitBtn.disabled = false;
            splitBtn.textContent = 'Split';
        }
    }, 20);
});

async function run() {
    try {
        await init();
        console.log('TileSplit WASM loaded');
    } catch (e) {
        showError(`Failed to load WASM module: ${e.message}. Build with: wasm-pack build --target web`);
    }
}

run();
