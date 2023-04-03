from winreg import REG_FULL_RESOURCE_DESCRIPTOR
from flask import Flask , request
import requests
import json
import datetime, time

app = Flask(__name__)

CAMERA = "http://192.168.128.169/"

right_eye_pos = 120
left_eye_pos =120
eye_dir = 1 # Initial direction for moveing eye

iteration = 0
rot_dir = 0 # Initial direction for rotating neck

max_sample_size = 4096

prev_sound ={} # Initialize sound buffer
for i in range(max_sample_size):
    prev_sound[i] = 8

@app.route('/')
def hello():
    name = "This is the neck commander for ChemiCo-SAN"
    return name

@app.route('/neck', methods=['POST'])
def neck_commander():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_raw = post1['battery_raw']
        global iteration
        global rot_dir , right_eye_pos , left_eye_pos
        duty = 45
        span = 20
        duty0 = 35
        slope0 =1
        duty1 = 35
        slope1 =1

        th_h = 30
        th_v = 20

        rsp = {}
        rsp['posting_interval'] = 1000
        rsp['rot_dir'] =  rot_dir
        rsp['duty'] =  duty
        rsp['span'] = span
        rsp['duty0'] = duty0
        rsp['duty1'] = duty1
        rsp['slope0'] = slope0
        rsp['slope1'] = slope1

    else:
        rsp = {}
        rsp['posting_interval'] = 2000
    return rsp

@app.route('/listener', methods=['POST'])
def listener():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        which_ear = post1['which']
        sample_time = post1['sample_time']
        sample_size = post1['sample_size']
        fft_data = post1['ear']
        global iteration , prev_sound , rot_dir
        global right_eye_pos
        global left_eye_pos
        iteration = iteration + 1
        rsp = {}
        rsp['posting_interval'] = 200
        rsp['cmd_right_eye'] =  right_eye_pos
        rsp['cmd_left_eye'] = left_eye_pos
        rsp['eye_move_delay'] = 1000
        
        if which_ear == 'right':
            rsp['which_ear'] = 'left'
            prev_sound = fft_data
        else:
            rsp['which_ear'] = 'right'
            rspns = decideSoundDir(fft_data , prev_sound)
            print(rspns)
            rot_dir = rspns['sound_dir']

    else:
        rsp = {}
        rsp['posting_interval'] = 2000
        rsp['which_ear'] = 'right'
    
    return rsp

def decideSoundDir(sound_data , prev_sound_data):
    low_limit = 20
    high_limit = 100
    loudness = 0
    th_q = 5
    sound_dir = 0
    for i in range(low_limit , high_limit):
        loudness += (sound_data[i] - prev_sound_data[i])
    loudness = loudness // (high_limit - low_limit)
    if loudness > th_q :
        sound_dir = 1
    elif loudness < -th_q:
        sound_dir = - 1
    else:
        sound_dir = 0
           
    rsp = {}
    rsp['sound_dir'] = sound_dir
    rsp['loudness'] = loudness
    
    return rsp

## for debug
if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', threaded=True)
