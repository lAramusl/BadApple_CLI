import cv2 
import numpy as np 
import os 
import time
import threading
from mutagen.wave import WAVE
from playsound import playsound

def play_music():
    try:
        playsound('bad_apple.wav')
    except Exception as e:
        print(f"Error playing sound: {e}")

def clear_screen(): # i found this on geeksforgeeks
    # For Windows
    if os.name == 'nt':
        _ = os.system('cls')
    # For macOS and Linux
    else:
        _ = os.system('clear')

audio_length = int(WAVE("bad_apple.wav").info.length)
video = cv2.VideoCapture("bad_apple.mp4")
#I WAS RIGHT ABOUT THE BLACK AND WHITE
#IN THE FINAL VERSION COLORS WERE INVERSED
white = 255 
black = 0 
whitePixel = '##' 
blackPixel = '  ' 

aspect_ratio = 4/3 
lines = os.get_terminal_size().lines#up-down
cols = os.get_terminal_size().columns#left-right
minLC = min(lines, cols)
maxLC = max(lines, cols)
width = int(minLC*aspect_ratio)
height = minLC

size = (width, height) 

horizontal_offset = (cols - (width*2))//2

if width > cols or height > lines:

    ans = input('Terminal resize is required, would you like to resize it? Y/N:  ')
    if ans.lower() == 'y' or ans.lower() == 'yes':
        os.system(f'mode con: cols={width*2} lines={height}')#resizes the terminal
        horizontal_offset = (cols - (width*2))//2 # recalculation is required
    else:
        print('The animation might be bugged then')

elif width * 2 > cols:
    print('Character width is lowered to fith the sizes')
    horizontal_offset = (cols - width)//2
    whitePixel = '#'
    blackPixel = ' '

#so it's less boring to just watch the animation
music_thread = threading.Thread(target=play_music, daemon=True)

modified_frames = np.empty(shape = (0))

#a counter for loading
frameNumber = 0

print('Loading...')
while True:#google says there are 6,562 frames in bad apple
    success, frame = video.read()

    if not success:
        break

    frame = cv2.resize(frame, size) 
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY) 
    _, thresholdedFrame = cv2.threshold(frame, 50, 255, cv2.THRESH_BINARY)  
    
    strFrame = str()
    for line in thresholdedFrame:

        strFrame += ' ' * horizontal_offset #shifting to middle

        for pixel in line: 
            strFrame += whitePixel if pixel == white else blackPixel

        strFrame += '\n'

    modified_frames = np.append(modified_frames, strFrame)
    frameNumber += 1
    print(f'frames: {frameNumber}/6572', end='\r')#google says there's 6562, strange

print(f"Generated {frameNumber} frames of 6572")
persec = 6572/audio_length
#i added loading to sync frames with music as computing some frames
#took more or less time than other frames.
#for example the scene with fire girl and 2 demon girls takes much more
#time to generate
#last 500 frames are really long to generate for some reason
#P.S took really long as the pc that i wrote the code on is from 2012
input('Loading complete, press enter to continue')

music_thread.start()

start_time = time.time()
for idx, frame in enumerate(modified_frames):
    print(frame, end='\r', flush=True)

    elapsed = time.time() - start_time
    target_time = idx / persec
    sleep_time = target_time - elapsed

    if sleep_time > 0:
        time.sleep(sleep_time)#matching the sound with picture
    #best 0.0285
#i've tried to match the sound and the picture perfectly,
#this is the best i've got, maybe others will not se the
#imperfections that exist here and that i see
#but still it was fun spending on synchronysing the whole day
#i was basically just picking methods and numbers for sync,
#very glad tho to add loading of frames seperately, as the animation with it looks smother
#11/24/2024, 2 days after last comments, seems like my old pc was just having lags with video
#so i changed the approach, very glad i figured out things that i didnt earlier
#now, everything is perfect, expect some windows terminals, best runs on powershell
clear_screen()
video.release()