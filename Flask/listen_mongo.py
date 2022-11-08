from winreg import REG_FULL_RESOURCE_DESCRIPTOR
from flask import Flask, request
from pymongo import MongoClient
import json
import datetime

app = Flask(__name__)

client = MongoClient('localhost', 27017)
db = client.ChemiCo_db
collection = db.listen_collection

session_id =1002
iteration = 0

@app.route('/')
def hello():
    name = "This is the listner for ChemiCo-SAN"
    return name

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
        iteration = iteration + 1
        post2 = {'session_id': session_id,
                 'which_ear': which_ear,
                 'sample_time': sample_time,
                 'sample_size': sample_size,
                 'fft_data': fft_data,
                 'iteration': iteration,
                 'date': datetime.datetime.utcnow()}
        result1 = collection.insert_one(post2)
        rsp = {}
        rsp['posting_interval'] = 200
        if which_ear == 'right':
            rsp['which_ear'] = 'left'
        else:
            rsp['which_ear'] = 'right'
    else:
        rsp = {}
        rsp['posting_interval'] = 2000
        rsp['which_ear'] = 'right'
    
    return rsp
    
## for debug
if __name__ == "__main__":
    app.run(debug=True, host='0.0.0.0')
