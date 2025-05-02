// 获取 DOM 元素
const videoContainer = document.getElementById('video-container');
const video = document.getElementById('videoPlayer');
const playPauseBtn = document.getElementById('play-pause-btn');
const playPauseIcon = playPauseBtn.querySelector('i');
const progressBar = document.getElementById('progress-bar');
const currentTimeDisplay = document.getElementById('current-time');
const durationDisplay = document.getElementById('duration');
const volumeBtn = document.getElementById('volume-btn');
const volumeIcon = volumeBtn.querySelector('i');
const volumeSlider = document.getElementById('volume-slider');
const fullscreenBtn = document.getElementById('fullscreen-btn');
const fullscreenIcon = fullscreenBtn.querySelector('i');

// --- 辅助函数 ---

// 格式化时间 (秒 -> MM:SS)
function formatTime(timeInSeconds) {
    if (isNaN(timeInSeconds)) {
        return '00:00';
    }
    const minutes = Math.floor(timeInSeconds / 60);
    const seconds = Math.floor(timeInSeconds % 60);
    return `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
}

// 更新播放/暂停按钮图标
function updatePlayPauseIcon() {
    if (video.paused || video.ended) {
        playPauseIcon.classList.remove('fa-pause');
        playPauseIcon.classList.add('fa-play');
        playPauseBtn.setAttribute('aria-label', '播放');
    } else {
        playPauseIcon.classList.remove('fa-play');
        playPauseIcon.classList.add('fa-pause');
        playPauseBtn.setAttribute('aria-label', '暂停');
    }
}

// 更新音量图标
function updateVolumeIcon() {
    volumeSlider.style.setProperty('--value', video.volume); // 可选：用于自定义滑块填充
    if (video.muted || video.volume === 0) {
        volumeIcon.classList.remove('fa-volume-up', 'fa-volume-down');
        volumeIcon.classList.add('fa-volume-mute');
        volumeBtn.setAttribute('aria-label', '取消静音');
    } else if (video.volume < 0.5) {
        volumeIcon.classList.remove('fa-volume-up', 'fa-volume-mute');
        volumeIcon.classList.add('fa-volume-down');
         volumeBtn.setAttribute('aria-label', '静音');
    } else {
        volumeIcon.classList.remove('fa-volume-down', 'fa-volume-mute');
        volumeIcon.classList.add('fa-volume-up');
         volumeBtn.setAttribute('aria-label', '静音');
    }
}

// --- 事件监听器 ---

// 播放/暂停 按钮点击
playPauseBtn.addEventListener('click', () => {
    if (video.paused || video.ended) {
        video.play();
    } else {
        video.pause();
    }
});

// 视频播放/暂停状态改变时更新按钮
video.addEventListener('play', updatePlayPauseIcon);
video.addEventListener('pause', updatePlayPauseIcon);
video.addEventListener('ended', updatePlayPauseIcon); // 播放结束后也更新

// 视频元数据加载完成后 (获取时长)
video.addEventListener('loadedmetadata', () => {
    const duration = video.duration;
    durationDisplay.textContent = formatTime(duration);
    progressBar.max = duration; // 设置进度条最大值
});

// 视频播放时间更新时
video.addEventListener('timeupdate', () => {
    const currentTime = video.currentTime;
    currentTimeDisplay.textContent = formatTime(currentTime);
    // 更新进度条的值 (仅在用户没有拖动时更新，避免冲突)
    if (!progressBar.matches(':active')) { // 检查用户是否正在与进度条交互
         progressBar.value = currentTime;
         // 可选: 更新进度条填充效果 (需要 CSS 配合)
         const percentage = (currentTime / video.duration) * 100;
         progressBar.style.setProperty('--value', `${percentage}%`);
    }
});

// 进度条输入/改变时 (用户拖动)
progressBar.addEventListener('input', () => {
    video.currentTime = progressBar.value;
    // 可选: 实时更新进度条填充效果
    const percentage = (progressBar.value / video.duration) * 100;
    progressBar.style.setProperty('--value', `${percentage}%`);
});
// 'change' 事件在用户释放滑块后触发，也可以用来设置时间
// progressBar.addEventListener('change', () => {
//     video.currentTime = progressBar.value;
// });


// 音量按钮点击 (静音/取消静音)
let lastVolume = 1; // 存储静音前的音量
volumeBtn.addEventListener('click', () => {
    if (video.muted) {
        video.muted = false;
        video.volume = lastVolume > 0.05 ? lastVolume : 0.1; // 恢复音量，避免恢复到0
        volumeSlider.value = video.volume;
    } else {
        lastVolume = video.volume; // 记录当前音量
        video.muted = true;
        volumeSlider.value = 0; // 将滑块设为0
    }
    updateVolumeIcon(); // 立即更新图标状态
});

// 音量条拖动
volumeSlider.addEventListener('input', () => {
    const newVolume = parseFloat(volumeSlider.value);
    video.volume = newVolume;
    video.muted = newVolume === 0; // 如果音量拖到0，则视为静音
    lastVolume = newVolume; // 更新最后记录的音量
    updateVolumeIcon();
});

// 视频音量改变时 (例如，通过快捷键改变) 更新滑块和图标
video.addEventListener('volumechange', () => {
    if (!video.muted) { // 只有在非静音状态下才更新滑块值
        volumeSlider.value = video.volume;
        lastVolume = video.volume; // 同时更新记录值
    } else {
        volumeSlider.value = 0; // 静音时滑块置0
    }
     updateVolumeIcon();
});


// 全屏按钮点击
fullscreenBtn.addEventListener('click', () => {
    // 检查当前是否处于全屏状态
    if (!document.fullscreenElement &&    // 标准 API
        !document.webkitFullscreenElement && // Safari/Chrome
        !document.mozFullScreenElement &&    // Firefox
        !document.msFullscreenElement) {     // IE/Edge
        // 进入全屏
        if (videoContainer.requestFullscreen) {
            videoContainer.requestFullscreen();
        } else if (videoContainer.webkitRequestFullscreen) { /* Safari/Chrome */
            videoContainer.webkitRequestFullscreen();
        } else if (videoContainer.mozRequestFullScreen) { /* Firefox */
            videoContainer.mozRequestFullScreen();
        } else if (videoContainer.msRequestFullscreen) { /* IE/Edge */
            videoContainer.msRequestFullscreen();
        }
         fullscreenIcon.classList.remove('fa-expand');
         fullscreenIcon.classList.add('fa-compress');
         fullscreenBtn.setAttribute('aria-label', '退出全屏');
    } else {
        // 退出全屏
        if (document.exitFullscreen) {
            document.exitFullscreen();
        } else if (document.webkitExitFullscreen) { /* Safari/Chrome */
            document.webkitExitFullscreen();
        } else if (document.mozCancelFullScreen) { /* Firefox */
            document.mozCancelFullScreen();
        } else if (document.msExitFullscreen) { /* IE/Edge */
            document.msExitFullscreen();
        }
         fullscreenIcon.classList.remove('fa-compress');
         fullscreenIcon.classList.add('fa-expand');
         fullscreenBtn.setAttribute('aria-label', '全屏');
    }
});

// 监听全屏状态变化事件 (例如按 ESC 退出全屏)
document.addEventListener('fullscreenchange', updateFullscreenIcon);
document.addEventListener('webkitfullscreenchange', updateFullscreenIcon);
document.addEventListener('mozfullscreenchange', updateFullscreenIcon);
document.addEventListener('MSFullscreenChange', updateFullscreenIcon);

function updateFullscreenIcon() {
     if (!document.fullscreenElement && !document.webkitFullscreenElement && !document.mozFullScreenElement && !document.msFullscreenElement) {
         fullscreenIcon.classList.remove('fa-compress');
         fullscreenIcon.classList.add('fa-expand');
         fullscreenBtn.setAttribute('aria-label', '全屏');
     } else {
         fullscreenIcon.classList.remove('fa-expand');
         fullscreenIcon.classList.add('fa-compress');
          fullscreenBtn.setAttribute('aria-label', '退出全屏');
     }
}


// --- 初始化 ---
// 页面加载时更新一次图标状态 (特别是音量)
updateVolumeIcon();
updatePlayPauseIcon();
updateFullscreenIcon(); // 确保初始图标正确

// 隐藏默认控件 (如果 video 标签意外添加了 controls 属性)
// video.removeAttribute('controls'); // 可以取消注释这行来强制移除