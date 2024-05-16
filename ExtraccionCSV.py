import serial
import time

file = open('D:\VSCODE\Sistemas Inteligentes II\Almacenamiento\Power Quality.txt', 'a')
espSer = serial.Serial('COM10', 115200)
time.sleep(1)

while True:
    data = espSer.readline().decode('utf-8')

    if data[0:4].isnumeric():
        print(data)
        file.write(data)