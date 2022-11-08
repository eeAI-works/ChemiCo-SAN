from pymongo import MongoClient
import json
import datetime
import matplotlib.pyplot as plt

client = MongoClient('localhost', 27017)
db = client.ChemiCo_db
collection = db.listen_collection

session_id = 1002
max_iteration = 32
iteration = 4
fft_x = 48
fft_y = 20

while iteration < max_iteration:
    ear_right = collection.find_one({'session_id':session_id , 'iteration': iteration})
    while ear_right['which_ear'] != 'right':
        iteration = iteration +1
        ear_right = collection.find_one({'session_id':session_id , 'iteration': iteration})
    iteration = iteration +1
    ear_left = collection.find_one({'session_id':session_id , 'iteration': iteration})
    while ear_left['which_ear'] != 'left':
        iteration = iteration +1
        ear_left = collection.find_one({'session_id':session_id , 'iteration': iteration})
    fft_right = ear_right['fft_data']
    fft_left = ear_left['fft_data']
    xy_list =[]
    for y in range(fft_y):
        xy_list.append(fft_right[y*fft_x:fft_x+y*fft_x])
    for y in range(fft_y):
        xy_list.append(fft_left[y*fft_x:fft_x+y*fft_x])
        plt.imshow(xy_list, origin='lower', cmap='nipy_spectral')
    print(iteration)
    plt.pause(0.01)