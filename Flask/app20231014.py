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

shoulder_r_pos = 200 #Initial Right Shoulder Position
shoulder_r_move = 0 # next shoulder moving amount
shoulder_r_dir = 0 # next shoulder moving direction
shoulder_r_servo = 135 #Initial Right Shoulder
shoulder_r_servo_dir = 1 # Initial Right Shoulder Directuion
elbow_r_servo =135 #Initial Right Elbow
elbow_r_servo_dir = 1 # Initial Right Elbow Direction
wing1_r = 90 #Initial Right Wing1
wing1_r_dir = 1 #Initial Right wing1 Direction
wing2_r = 90 #Initial Right Wing1
wing2_r_dir = 1 #Initial Right wing1 Direction

shoulder_l_pos = 200 #Initial Left Shoulder Position
shoulder_l_move = 0 # next shoulder moving amount
shoulder_l_dir = 0 # next shoulder moving direction
shoulder_l_servo = 45 #Initial Left Shoulder
shoulder_l_servo_dir = 1 # Initial Left Shoulder Directuion
elbow_l_servo =45 #Initial Left Elbow
elbow_l_servo_dir = 1 # Initial Left Elbow Direction
wing1_l = 90 #Initial Left Wing1
wing1_l_dir = 1 #Initial Left wing1 Direction
wing2_l = 90 #Initial Left Wing1
wing2_l_dir = 1 #Initial Left wing1 Direction

#Common hand Param
wrist1_r = 90
wrist1_r_dir = 1
wrist2_r = 90
wrist2_r_dir = 1
wrist1_l = 90
wrist1_l_dir = 1
wrist2_l = 90
wrist2_l_dir = 1
fan_span =500
fan_cont = True
fan_duty = 250
fan_dir = 1

prev_sound ={} # Initialize sound buffer
for i in range(max_sample_size):
    prev_sound[i] = 8

tire_dir = [1,1,1,1,1,1,1,1]
foot_cont = True
foot_rotate = False
foot_span = 300

@app.route('/')
def hello():
    name = "This is the local AI server for ChemiCo-SAN"
    return name

@app.route('/hello')
def hello2():
    name = "Hello, The World"
    return name

@app.route('/foot135', methods=['POST'])
def foot135():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_voltage15 = post1['battery_voltage15']
        battery_voltage3 = post1['battery_voltage3']
        scale1 = post1['scale1']
        scale3 = post1['scale3']
        scale5 = post1['scale5']
        print('Battery15:',battery_voltage15,'V Battery3:',battery_voltage3,'V Scale1:',scale1,'g Scale3:',scale3,'g Scale5:',scale5,'g') 
        global iteration
        global foot_cont
        global foot_span

        tire1_cont=foot_cont
        tire3_cont=foot_cont
        tire5_cont=foot_cont
        tire1_spd=255
        tire3_spd=255
        tire5_spd=255
        tire1_dir=decideTireDir(1)
        tire3_dir=decideTireDir(3)
        tire5_dir=decideTireDir(5)
        period = iteration  % 2
        if period == 0 and foot_rotate == False:
            tire3_dir = decideTireDir(3)
            # tire3_cont=False
        if period == 1 and foot_rotate == False:
            tire3_dir = -decideTireDir(3)
            # tire3_cont=False

        rsp = {}
        rsp['posting_interval'] = 500
        rsp['span'] = foot_span
        rsp['cont1'] = tire1_cont
        rsp['duty1'] = tire1_spd
        rsp['rot_dir1'] = tire1_dir
        rsp['cont3'] = tire3_cont
        rsp['duty3'] = tire3_spd
        rsp['rot_dir3'] = tire3_dir
        rsp['cont5'] = tire5_cont
        rsp['duty5'] = tire5_spd
        rsp['rot_dir5'] = tire5_dir

        iteration += 1

        return rsp

@app.route('/foot24', methods=['POST'])
def foot24():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_voltage24 = post1['battery_voltage24']
        scale2 = post1['scale2']
        scale4 = post1['scale4']
        print('Battery24:',battery_voltage24,'V Scale2:',scale2,'g Scale4:',scale4,'g')
        global iteration
        global foot_cont
        global foot_span
        tire2_cont=foot_cont
        tire4_cont=foot_cont
        tire2_spd=255
        tire4_spd=255
        tire2_dir=decideTireDir(2)
        tire4_dir=decideTireDir(4)
        
        rsp = {}
        rsp['posting_interval'] = 500
        rsp['span'] = foot_span
        rsp['cont2'] = tire2_cont
        rsp['duty2'] = tire2_spd
        rsp['rot_dir2'] = tire2_dir
        rsp['cont4'] = tire4_cont
        rsp['duty4'] = tire4_spd
        rsp['rot_dir4'] = tire4_dir

        # iteration += 1
        return rsp
    
@app.route('/upr', methods=['GET'])
def liftup_r():
    global shoulder_r_pos
    global shoulder_r_move
    global shoulder_r_dir
    max_shoulder_r_pos = 250 # max shoulder Position
    length = request.args.get("length", default=0, type=int)
    print('length:',length,'mm')
    if (shoulder_r_pos + length) > max_shoulder_r_pos:
        shoulder_r_move = max_shoulder_r_pos - shoulder_r_pos
    elif (shoulder_r_pos + length) < 0:
        shoulder_r_move = 0 - shoulder_r_pos
    else:
        shoulder_r_move = length
    shoulder_r_dir = 0
    if shoulder_r_move > 0:
        shoulder_r_dir = 1
    elif shoulder_r_move < 0:
        shoulder_r_dir = -1
        shoulder_r_move = abs(shoulder_r_move)
    
    rsp={}
    rsp['posting_interval'] = 2000
    rsp['shoulder_r_move'] = shoulder_r_move
    rsp['shoulder_r_dir'] = shoulder_r_dir

    return rsp

@app.route('/shoulderR', methods=['POST'])
def shoulder_right():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_voltage = post1['battery_voltage']
        print('Battery:',battery_voltage,'V')
        global iteration
        global shoulder_r_pos
        global shoulder_r_move
        global shoulder_r_dir

        duty = shoulder_r_move
        direction = shoulder_r_dir
        shoulder_r_move = 0
        shoulder_r_dir = 0
        slope = 120
    
        global shoulder_r_servo
        global shoulder_r_servo_dir
        max_shoulder_r_servo = 165 # maximum Right Shoulder
        min_shoulder_r_servo = 105 # minimum Right Shoulder
        global elbow_r_servo
        global elbow_r_servo_dir
        max_elbow_r_servo = 165 # maximum Right Shoulder
        min_elbow_r_servo = 105# minimum Right Shoulder
        global wing1_r
        global wing1_r_dir
        max_wing1_r = 100
        min_wing1_r = 80
        global wing2_r
        global wing2_r_dir
        max_wing2_r = 100
        min_wing2_r = 80

        shoulder_r_servo  += shoulder_r_servo_dir * 2
        if shoulder_r_servo > max_shoulder_r_servo:
            shoulder_r_servo_dir = -1
        if shoulder_r_servo < min_shoulder_r_servo:
            shoulder_r_servo_dir = 1
        elbow_r_servo  += elbow_r_servo_dir * 2
        if elbow_r_servo > max_elbow_r_servo:
            elbow_r_servo_dir = -1
        if elbow_r_servo < min_elbow_r_servo:
            elbow_r_servo_dir = 1
        wing1_r  += wing1_r_dir * 0
        if wing1_r > max_wing1_r:
            wing1_r_dir = -1
        if wing1_r < min_wing1_r:
            wing1_r_dir = 1
        wing2_r  += wing2_r_dir * 1
        if wing2_r > max_wing2_r:
            wing2_r_dir = -1
        if wing2_r < min_wing2_r:
            wing2_r_dir = 1        

        rsp = {}
        rsp['posting_interval'] = 500
        rsp['duty'] = duty
        rsp['slope'] = slope
        rsp['direction'] = direction
        rsp['pos1'] = shoulder_r_servo
        rsp['pos2'] = elbow_r_servo
        rsp['pos3'] = wing1_r
        rsp['pos4'] = wing2_r
    else:
        rsp = {}
        rsp['posting_interval'] = 2000
    return rsp

    
@app.route('/upl', methods=['GET'])
def liftup_l():
    global shoulder_l_pos
    global shoulder_l_move
    global shoulder_l_dir
    max_shoulder_l_pos = 250 # max shoulder Position
    length = request.args.get("length", default=0, type=int)
    print('length:',length,'mm')
    if (shoulder_l_pos + length) > max_shoulder_l_pos:
        shoulder_l_move = max_shoulder_l_pos - shoulder_l_pos
    elif (shoulder_l_pos + length) < 0:
        shoulder_l_move = 0 - shoulder_l_pos
    else:
        shoulder_l_move = length
    shoulder_l_dir = 0
    if shoulder_l_move > 0:
        shoulder_l_dir = 1
    elif shoulder_l_move < 0:
        shoulder_l_dir = -1
        shoulder_l_move = abs(shoulder_l_move)
    
    rsp={}
    rsp['posting_interval'] = 2000
    rsp['shoulder_l_move'] = shoulder_l_move
    rsp['shoulder_l_dir'] = shoulder_l_dir

    return rsp

@app.route('/shoulderL', methods=['POST'])
def shoulder_left():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_voltage = post1['battery_voltage']
        print('Battery:',battery_voltage,'V')
        global iteration
        global shoulder_l_pos
        global shoulder_l_move
        global shoulder_l_dir
 
        duty = shoulder_l_move
        direction = shoulder_l_dir
        shoulder_l_pos += duty * direction
        shoulder_l_move = 0
        shoulder_l_dir = 0
        slope = 120
    
        global shoulder_l_servo
        global shoulder_l_servo_dir
        max_shoulder_l_servo = 75 # maximum Left Shoulder
        min_shoulder_l_servo = 15 # minimum Left Shoulder
        global elbow_l_servo
        global elbow_l_servo_dir
        max_elbow_l_servo = 75 # maximum Left Shoulder
        min_elbow_l_servo = 15# minimum Left Shoulder
        global wing1_l
        global wing1_l_dir
        max_wing1_l = 100
        min_wing1_l = 80
        global wing2_l
        global wing2_l_dir
        max_wing2_l = 100
        min_wing2_l = 80

        shoulder_l_servo  += shoulder_l_servo_dir * 2
        if shoulder_l_servo > max_shoulder_l_servo:
            shoulder_l_servo_dir = -1
        if shoulder_l_servo < min_shoulder_l_servo:
            shoulder_l_servo_dir = 1
        elbow_l_servo  += elbow_l_servo_dir * 2
        if elbow_l_servo > max_elbow_l_servo:
            elbow_l_servo_dir = -1
        if elbow_l_servo < min_elbow_l_servo:
            elbow_l_servo_dir = 1
        wing1_l  += wing1_l_dir * 0
        if wing1_l > max_wing1_l:
            wing1_l_dir = -1
        if wing1_l < min_wing1_l:
            wing1_l_dir = 1
        wing2_l  += wing2_l_dir * 1
        if wing2_l > max_wing2_l:
            wing2_l_dir = -1
        if wing2_l < min_wing2_l:
            wing2_l_dir = 1        

        rsp = {}
        rsp['posting_interval'] = 500
        rsp['duty'] = duty
        rsp['slope'] = slope
        rsp['direction'] = direction
        rsp['pos1'] = shoulder_l_servo
        rsp['pos2'] = elbow_l_servo
        rsp['pos3'] = wing1_l
        rsp['pos4'] = wing2_l
    else:
        rsp = {}
        rsp['posting_interval'] = 2000
    return rsp

@app.route('/handR', methods=['POST'])
def hand_right():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_voltage = post1['battery_voltage']
        temperature = post1['temperature']
        temp_unit = post1['temp_unit']
        pressure = post1['pressure']
        pres_unit = post1['pres_unit']
        humidity = post1['humidity']
        print('Battery:',battery_voltage,'V , Temperature:',temperature,'degC , Pressure:',pressure,'Pa , Humidity:',humidity)
        global iteration
        global wrist1_r
        global wrist1_r_dir
        global wrist2_r
        global wrist2_r_dir
        global fan_span
        global fan_cont 
        global fan_duty
        global fan_dir
        max_wrist1 = 150
        min_wrist1 = 45
        max_wrist2 = 110
        min_wrist2 = 70

        wrist1_r  += wrist1_r_dir* 2
        if wrist1_r > max_wrist1:
            wrist1_r_dir = -1
        if wrist1_r < min_wrist1:
            wrist1_r_dir = 1
        wrist2_r  += wrist2_r_dir * 1
        if wrist2_r > max_wrist2:
            wrist2_r_dir = -1
        if wrist2_r < min_wrist2:
            wrist2_r_dir = 1

        rsp = {}
        rsp['posting_interval'] = 500
        rsp['pos1'] = wrist1_r
        rsp['pos2'] = wrist2_r
        rsp['span'] = fan_span
        rsp['duty1'] = fan_duty
        rsp['cont1'] = fan_cont
        rsp['rot_dir1'] = fan_dir
        rsp['duty2'] = fan_duty
        rsp['cont2'] = fan_cont
        rsp['rot_dir2'] = fan_dir
        rsp['duty3'] = fan_duty
        rsp['cont3'] = fan_cont
        rsp['rot_dir3'] = fan_dir
        rsp['duty4'] = fan_duty
        rsp['cont4'] = fan_cont
        rsp['rot_dir4'] = fan_dir

    else:
        rsp = {}
        rsp['posting_interval'] = 2000
    return rsp
    
@app.route('/handL', methods=['POST'])
def hand_left():
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_voltage = post1['battery_voltage']
        temperature = post1['temperature']
        temp_unit = post1['temp_unit']
        pressure = post1['pressure']
        pres_unit = post1['pres_unit']
        humidity = post1['humidity']
        print('Battery:',battery_voltage,'V , Temperature:',temperature,'degC , Pressure:',pressure,'Pa , Humidity:',humidity)
        global iteration
        global wrist1_l
        global wrist1_l_dir
        global wrist2_l
        global wrist2_l_dir
        global fan_span
        global fan_cont 
        global fan_duty
        global fan_dir
        max_wrist1 = 135
        min_wrist1 = 30
        max_wrist2 = 110
        min_wrist2 = 70

        wrist1_l  += wrist1_l_dir* 2
        if wrist1_l > max_wrist1:
            wrist1_l_dir = -1
        if wrist1_l < min_wrist1:
            wrist1_l_dir = 1
        wrist2_l  += wrist2_l_dir * 1
        if wrist2_l > max_wrist2:
            wrist2_l_dir = -1
        if wrist2_l < min_wrist2:
            wrist2_l_dir = 1

        rsp = {}
        rsp['posting_interval'] = 500
        rsp['pos1'] = wrist1_l
        rsp['pos2'] = wrist2_l
        rsp['span'] = fan_span
        rsp['duty1'] = fan_duty
        rsp['cont1'] = fan_cont
        rsp['rot_dir1'] = fan_dir
        rsp['duty2'] = fan_duty
        rsp['cont2'] = fan_cont
        rsp['rot_dir2'] = fan_dir
        rsp['duty3'] = fan_duty
        rsp['cont3'] = fan_cont
        rsp['rot_dir3'] = fan_dir
        rsp['duty4'] = fan_duty
        rsp['cont4'] = fan_cont
        rsp['rot_dir4'] = fan_dir

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
        iteration += 1
        # rot_dir= (iteration % 3 ) - 1
        duty = 150
        span = 20
        duty0 = 80
        slope0 =1
        duty1 = 80
        slope1 =1

        th_h = 30
        th_v = 20

        voice_cmd_num = 5
        loudness=[200,250,200,200,200]
        voice_cords= [65,70,65,70,65]
        tongue=[20,20,20,20,20]
        lips=[80,80,80,80,80]
        voice_delay=[200,300,500,500,500]
        
        # loudness= random.randint(150,250)
        # voice_cords= random.randint(40,70)
        # start_voice_cords=voice_cords-5+random.randint(0,90)
        # start_voice_cords=voice_cords
        # tongue=random.randint(5,45)
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
        rsp['voice_cmd_num'] =voice_cmd_num
        rsp['loudness'] = loudness
        rsp['voice_cords'] = voice_cords
        rsp['tongue'] = tongue
        rsp['lips'] = lips
        rsp['voice_delay'] = voice_delay

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
        global eye_dir
        max_eye_pos = 140
        min_eye_pos = 90
        right_eye_pos += eye_dir * 1
        if right_eye_pos > max_eye_pos:
            eye_dir = -1
        if right_eye_pos < min_eye_pos:
            eye_dir = 1
        left_eye_pos += eye_dir * 1
        if left_eye_pos > max_eye_pos:
            eye_dir = -1
        if left_eye_pos < min_eye_pos:
            eye_dir = 1


        iteration = iteration + 1

        rsp = {}
        rsp['posting_interval'] = 200
        rsp['cmd_right_eye'] =  right_eye_pos
        rsp['cmd_left_eye'] = left_eye_pos
        rsp['eye_move_delay'] = 500
        
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

def decideTireDir(tire_num):
    global iteration
    global foot_rotate
    period = (iteration // 10) % 2
    if period == 0:
        dir = tire_dir[tire_num]
        if tire_num == 1 and foot_rotate == False:
            dir = -tire_dir[tire_num]
        if tire_num == 2 and foot_rotate == False:
            dir = -tire_dir[tire_num]
        # if tire_num == 3 and foot_rotate == False:
            # dir = 0
        # if tire_num == 2 and foot_rotate == False:
        #     dir = 0
    if period == 1:
        dir = -tire_dir[tire_num]
        if tire_num == 1 and foot_rotate == False:
            dir = tire_dir[tire_num]
        if tire_num == 2 and foot_rotate == False:
            dir = tire_dir[tire_num]
        # if tire_num == 3 and foot_rotate == False:
            # dir = 0
        # if tire_num == 4 and foot_rotate == False:
        #     dir = 0
    return dir


# for debug
if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', threaded=True)
