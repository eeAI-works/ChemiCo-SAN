from pymongo import MongoClient
import json
import datetime
import matplotlib.pyplot as plt

client = MongoClient('localhost', 27017)
db = client.ChemiCo_db
collection = db.listen_collection

session_id = 1002
max_iteration = 32
start_iteration = 4

iteration = start_iteration
while iteration < max_iteration:
    ear= collection.find_one({'session_id':session_id , 'iteration': iteration})
    which_ear = ear['which_ear']
    if which_ear == 'right':
        sampling_size = ear['sample_size']
        sampling_time = ear['sample_time']
        fft_bin = []
        fft_data = ear['fft_data']
        print(str(iteration)+' '+which_ear)
        for i in range(len(fft_data)):
            fft_bin.append( 1000 * i / sampling_time)
        plt.xscale('log') 
        plt.title(which_ear + ' ear')
        plt.plot(fft_bin,fft_data)
        plt.pause(0.5)
    iteration = iteration +1
    
plt.show()
iteration = start_iteration
while iteration < max_iteration:
    ear= collection.find_one({'session_id':session_id , 'iteration': iteration})
    which_ear = ear['which_ear']
    if which_ear == 'left':
        sampling_size = ear['sample_size']
        sampling_time = ear['sample_time']
        fft_bin = []
        fft_data = ear['fft_data']
        print(str(iteration)+' '+which_ear)
        for i in range(len(fft_data)):
            fft_bin.append( 1000 * i / sampling_time)
        plt.xscale('log') 
        plt.title(which_ear + ' ear')
        plt.plot(fft_bin,fft_data)
        plt.pause(0.5)
    iteration = iteration +1
plt.show()