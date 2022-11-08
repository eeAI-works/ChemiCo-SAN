import requests
import matplotlib.pyplot as plt

CAMERA = "http://192.168.128.173/"
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

z_level = 3
pan_level = 3
tilt_level = 3
for i in range(pan_level):
    for j in range(tilt_level):
        for k in range(z_level):
            uri      = "vision"
            zoom     = k *2  + max_zoom / 3
            pan      = center_x + (i - 1) * px
            tilt     = center_y + (j - 1) * py
            param  = "?zoom=" +str(zoom) +"&pan=" + str(pan) + "&tilt=" + str(tilt) 
            res = requests.get(CAMERA + uri + param)
            cam_j = res.json()
            cam_x = cam_j['x']
            cam_y = cam_j['y']
            pix_list = cam_j['pixel']
            xy_list =[]
            for y in range(cam_y):
                xy_list.append(pix_list[y*cam_x:cam_x+y*cam_x])
                plt.imshow(xy_list)
            plt.pause(0.01)