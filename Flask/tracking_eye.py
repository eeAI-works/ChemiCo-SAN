from winreg import REG_FULL_RESOURCE_DESCRIPTOR
from flask import Flask , request
import requests
import json
import datetime, time

app = Flask(__name__)

CAMERA = "http://192.168.128.169/"
uri = "param"
zoom = 100 # More than Max zoom Parameter
param ="?zoom=" +str(zoom)
res = requests.get(CAMERA + uri + param)
cam_j = res.json()
max_x = cam_j['x']
max_y = cam_j['y']
px = cam_j['px']
py = cam_j['py']
max_zoom = cam_j['max_zoom']
center_x = cam_j['pan']
center_y = cam_j['tilt']
min_pix_size = cam_j['pix_size']

zoom = 8 # eye zoom
right_eye_pos = 120
left_eye_pos =120
eye_dir = 1 # Initial direction for moveing eye

iteration = 0
rot_dir = 0 # Initial direction for rotating neck

@app.route('/')
def hello():
    name = "This is the neck commander for ChemiCo-SAN"
    return name

@app.route('/neck', methods=['POST'])
def neck_commander():
    print('request recived')
    if request.method == 'POST':
        json1 = request.data.decode('utf-8')
        post1 = json.loads(json1)
        battery_raw = post1['battery_raw']
        global CAMERA,iteration
        global rotdir , right_eye_pos , left_eye_pos
        iteration +=1
        duty = 45
        span = 20
        duty0 = 35
        slope0 =1
        duty1 = 35
        slope1 =1

        th_h = 20
        th_v = 20

        area = div4Vision(iteration , CAMERA)
        l_w =area['area0'] + area['area2']
        r_w =area['area1'] + area['area3']
        if (l_w - r_w) >  th_h:
            rot_dir = +1
        elif (r_w - l_w) > th_h:
            rot_dir = -1
        else:
            rot_dir = 0
        
        u_w =area['area0'] + area['area1']
        d_w =area['area2'] + area['area3']
        if (u_w - d_w) > th_v:
            right_eye_pos -= 5
        elif (d_w - u_w) > th_v:
            right_eye_pos += 5
        left_eye_pos = right_eye_pos


        rsp = {}
        rsp['posting_interval'] = 200
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
        global iteration
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
        else:
            rsp['which_ear'] = 'right'
    else:
        rsp = {}
        rsp['posting_interval'] = 2000
        rsp['which_ear'] = 'right'
    
    return rsp

def div4Vision(iteration, CAMERA):
    uri      = "vision"
    zoom     = 4
    th0 = 90 # 90/255 (35%)
    st_t=time.time()
    param  = "?zoom=" +str(zoom)
    res = requests.get(CAMERA + uri + param)
    re_t=time.time()
    cam_j = res.json()
    cam_x = cam_j['x']
    cam_y = cam_j['y']
    pix_list = cam_j['pixel']
    pix_q_list = []
    xy_list=[]
    for i in range(len(pix_list)):
        pix_v = sum(pix_list[i])
        if pix_v > th0*3:
            pix_q_list.append(1)
        else:
            pix_q_list.append(0)
    len4 = len(pix_q_list)//4
    area0 = sum(pix_q_list[0:len4])
    area1 = sum(pix_q_list[len4:len4*2])
    area2 = sum(pix_q_list[len4*2:len4*3])
    area3 = sum(pix_q_list[len4*3:len4*4])
    en_t =time.time()
    print('a0=',area0,', a1=',area1,', a2=',area2,', a3=',area3)
    print('iter.=',iteration,', req_t=',re_t-st_t,', other=',en_t-re_t)
    
    rsp = {}
    rsp['area0'] = area0
    rsp['area1'] = area1
    rsp['area2'] = area2
    rsp['area3'] = area3
    
    return rsp

## for debug
if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0', threaded=True)
