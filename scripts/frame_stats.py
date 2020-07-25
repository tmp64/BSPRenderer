import matplotlib.pyplot as plt
import numpy as np
import sys
from typing import List
from typing import Tuple


# Splits string into an array of ints
def parse_text_line_int(line: str) -> List[int]:
    line_split = line.strip().split()
    ret = []
    for i in line_split:
        if len(i.strip()) != 0:
            ret.append(int(i.strip()))
    return ret


# Load and parse file
time = []       # Time since start of playback in seconds
frametime = []  # Frametime in milliseconds
wsurf = []      # Number of world surfaces in a frame

with open(sys.argv[1], "r") as file:
    skip = 1
    for line in file:
        if skip > 0:
            skip -= 1   # Skip N frames
            continue

        data = parse_text_line_int(line)
        time.append(data[0] / 1000000.0)
        frametime.append(data[1] / 1000.0)
        wsurf.append(data[2])


# Show frametimes graph
def show_frame_times():
    fig, ax = plt.subplots()
    fig.set_figwidth(6)
    fig.set_figheight(4)

    plt.scatter(time, frametime, s=0.5)
    plt.xlabel("Time (seconds)")
    plt.ylabel("Frametime (ms)")


# Show framerate graph
def show_frame_rate():
    fig, ax = plt.subplots()
    fig.set_figwidth(6)
    fig.set_figheight(4)

    plt.plot(time, [1000 / i for i in frametime])
    plt.xlabel("Time (seconds)")
    plt.ylabel("Framerate (FPS)")


show_frame_times()
show_frame_rate()
plt.show()
