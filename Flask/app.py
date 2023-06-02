from flask import Flask , request
import requests
import json
import datetime, time
import random

app = Flask(__name__)

right_eye_pos = 120
left_eye_pos =120
eye_dir = 1 # Initial direction for moveing eye

iteration = 0
rot_dir = 0 # Initial direction for rotating neck

max_sample_size = 4096

neck_pos = 0 #Initial Neck Height
neck_move = 0 # next neck moving amount
neck_dir = 0 # next neck moving direction

prev_sound ={} # Initialize sound buffer
for i in range(max_sample_size):
    prev_sound[i] = 8

@app.route('/')
def hello():
    name = "This is the Voice Generator for ChemiCo-SAN"
    return name

@app.route('/girafe_neck', methods=['GET'])
def girafe_neck():
    global neck_pos
    global neck_move
    global neck_dir
    max_neck_pos = 110
    length = request.args.get("length", default=0, type=int)
    if (neck_pos + length) > max_neck_pos:
        neck_move = max_neck_pos - neck_pos
    elif (neck_pos + length) < 0:
        neck_move = 0 - neck_pos
    else:
        neck_move = length
    neck_dir = 0
    if neck_move > 0:
        neck_dir = 1
    elif neck_move < 0:
        neck_dir = -1
        neck_move = abs(neck_move)
    
    rsp={}
    rsp['posting_interval'] = 2000
    rsp['neck_move'] = neck_move
    rsp['neck_dir'] = neck_dir

    return rsp

@app.route('/shoulder', methods=['POST'])
def liftmove():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_voltage0 = post1['battery_voltage0']
        print('Battery0:',battery_voltage0,'V')
        battery_voltage1 = post1['battery_voltage1']
        print('Battery1:',battery_voltage1,'V')
        global iteration
        global neck_pos
        global neck_move
        global neck_dir
        duty = neck_move
        direction = neck_dir
        neck_pos += duty * direction
        neck_move = 0
        neck_dir = 0
        slope = 120
        
        rsp = {}
        rsp['posting_interval'] = 10000
        rsp['duty'] = duty
        rsp['slope'] = slope
        rsp['direction'] = direction
        
    else:
        rsp = {}
        rsp['posting_interval'] = 2000
    return rsp
        
@app.route('/neck', methods=['POST'])
def neck_commander():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_voltage = post1['battery_voltage']
        print('Battery:',battery_voltage,'V')
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

        bless =5000
        loudness=200
        voice_cords= 50
        start_voice_cords=50
        tongue=50
        start_tongue=50
        lips=70
        start_lips=70
        servo_delay=200
        
        bless = random.randint(100,2000)
        loudness= random.randint(150,250)
        voice_cords= random.randint(40,70)
        start_voice_cords=voice_cords-5+random.randint(0,90)
        # start_voice_cords=voice_cords
        # tongue=random.randint(10,95)
        # start_tongue=tongue-5+random.randint(0,10)
        # start_tongue=tongue
        # lips=random.randint(75,105)
        # start_lips=lips-5+random.randint(0,10)
        # start_lips=lips
        servo_delay=random.randint(10,200)

        rsp = {}
        rsp['posting_interval'] = 1000
        rsp['rot_dir'] =  rot_dir
        rsp['duty'] =  duty
        rsp['span'] = span
        rsp['duty0'] = duty0
        rsp['duty1'] = duty1
        rsp['slope0'] = slope0
        rsp['slope1'] = slope1
        rsp['bless'] = bless
        rsp['loudness'] = loudness
        rsp['voice_cords'] = voice_cords
        rsp['start_voice_cords'] = start_voice_cords
        rsp['tongue'] = tongue
        rsp['start_tongue'] = start_tongue
        rsp['lips'] = lips
        rsp['start_lips'] = start_lips
        rsp['servo_delay'] = servo_delay

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



# for debug
if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', threaded=True)
