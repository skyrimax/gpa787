# -*- coding: utf-8 -*-
"""
Created on Fri Oct  2 12:26:14 2020

@author: ggauthier
"""

import pygame
import serial
import time

fermeApp = False
serialStringIn = ""
newDataAvailable = False
action = False

BLACK = (0,0,0)
WHITE = (255,255,255)
GREEN = (0,255,0)
RED = (255,0,0)
BLUE = (0,0,255)
MAROON = (128,0,0)
ORANGE = (231,81,0)
JAUNE = (255,235,59)
VERT = (205,255,204)
GRIS = (158,158,158)
GRIS2 = (58,58,58)
BLUEPALE = (142, 192, 219)
TOMATE = (255, 99, 71)

timeUnit = "ms"
timeBase = 0.50
timeK = 16
timeSca = 1
timeBaseF = timeBase*timeK
onde = 0        # sinusoide (1 - carré)
ampli = 2.0
freq = 16.0

pygame.init()

pygame.font.init()
myfont = pygame.font.SysFont('arialblack', 22)

size = (575,500)
screen = pygame.display.set_mode(size)
pygame.display.set_caption("Oscilloscope")

serialPort = serial.Serial()
serialPort.baudrate = 250000 #115200
serialPort.port = "/dev/ttyACM0"
serialPort.timeout = 0.05


serialPort.open()

while fermeApp == False:
    
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            fermeApp = True
            serialPort.close()
            pygame.display.quit()
            pygame.quit()
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_u:
                action = True
                touche = "U";
                #print("U demandé ---> ")
            if event.key == pygame.K_d:
                action = True
                touche = "D";
            if event.key == pygame.K_e:
                action = True
                touche = "E";
            if event.key == pygame.K_r:
                action = True
                touche = "R";
            if event.key == pygame.K_1:
                action = True
                touche = "1";
            if event.key == pygame.K_2:
                action = True
                touche = "2";
            if event.key == pygame.K_3:
                action = True
                touche = "3";
            if event.key == pygame.K_4:
                action = True
                touche = "4";
            if event.key == pygame.K_5:
                action = True
                touche = "5";
            if event.key == pygame.K_6:
                action = True
                touche = "6";
            if event.key == pygame.K_7:
                action = True
                touche = "7";
            if event.key == pygame.K_8:
                action = True
                touche = "8";
            if event.key == pygame.K_s:
                action = True
                touche = "S";
            if event.key == pygame.K_a:
                action = True
                touche = "A";
            if event.key == pygame.K_w:
                action = True
                touche = "W";
            if event.key == pygame.K_m:
                action = True
                touche = "M";
            if event.key == pygame.K_n:
                action = True
                touche = "N";
            if event.key == pygame.K_b:
                action = True
                touche = "B";
            if event.key == pygame.K_t:
                action = True
                touche = "T";
        ##if event.type == pygame.MOUSEBUTTONUP:
        ##    pos = pygame.mouse.get_pos()
        ##    print(pos)
        ##    if ((pos[0]>200) & (pos[0]<300) & (pos[1]>150) & (pos[1]<200)):
        ##        print('Ici')

            
    if (serialPort.in_waiting>0):
        serialStringIn = serialPort.readline()
        if (serialStringIn[:len(serialStringIn)-2] == b'R?'):
            serialPort.reset_input_buffer()
            serialPort.write(b"K")
            while(serialPort.in_waiting<100):
                time.sleep(0.00001)
                
            data = serialPort.read(101)
            while(serialPort.in_waiting<100):
                time.sleep(0.00001)
                
            dataGen = serialPort.read(101)
            newDataAvailable = True
            if action == True:
                if touche == "U": 
                    serialPort.write(b"U")
                    timeK = timeK+1
                    timeBaseF = timeBase*timeK
                    print("U envoyé --->")
                if touche == "D": 
                    serialPort.write(b"D")
                    if timeK>1:
                        timeK = timeK-1
                    timeBaseF = timeBase*timeK
                if touche == "1": 
                    serialPort.write(b"1")
                    timeK = 1
                    timeBaseF = timeBase*timeK
                if touche == "2": 
                    serialPort.write(b"2")
                    timeK = 2
                    timeBaseF = timeBase*timeK
                if touche == "3": 
                    serialPort.write(b"3")
                    timeK = 4
                    timeBaseF = timeBase*timeK
                if touche == "4": 
                    serialPort.write(b"4")
                    timeK = 8
                    timeBaseF = timeBase*timeK
                if touche == "5": 
                    serialPort.write(b"5")
                    timeK = 16
                    timeBaseF = timeBase*timeK
                if touche == "6": 
                    serialPort.write(b"6")
                    timeK = 32
                    timeBaseF = timeBase*timeK
                if touche == "7": 
                    serialPort.write(b"7")
                    timeK = 64
                    timeBaseF = timeBase*timeK
                if touche == "8": 
                    serialPort.write(b"8")
                    timeK = 128
                    timeBaseF = timeBase*timeK
                if touche == "S": 
                    serialPort.write(b"S")
                    freq *= 2.0
                    if (freq>256):
                        freq=256
                if touche == "A": 
                    serialPort.write(b"A")
                    freq /= 2.0
                    if (freq<0.250):
                        freq=0.25
                if touche == "W": 
                    serialPort.write(b"W")
                    onde = onde+1
                    if (onde>1):
                        onde=0
                if touche == "E": 
                    serialPort.write(b"E")
                    ampli-=0.25;
                    if (ampli<0.25):
                        ampli = 0.25
                if touche == "R": 
                    serialPort.write(b"R")
                    ampli+=0.25
                    if (ampli>2.5):
                        ampli = 2.5
                if touche == "M": 
                    timeSca = timeSca+1
                if touche == "B":
                    if (timeSca>1):
                        timeSca = timeSca-1
                if touche == "N": 
                    timeSca = 1
                if touche == "T": 
                    serialPort.write(b"T")
                    
                action = False
            else:
                serialPort.write(b"K")
            
            moyenne = sum(data)/len(data)
            
            valeur_Min = 255
            valeur_Max = 0
            
            for i in range(0,100):
                if(data[i]>valeur_Max):
                    valeur_Max = data[i]
                    
                if(data[i]<valeur_Min):
                    valeur_Min = data[i]
                    
        serialPort.reset_input_buffer()
        
    if newDataAvailable == True:
        screen.fill(BLACK)
        
        N = int(100/timeSca)
        
        for i in range(0,N+1):
            if(i<N):
                pygame.draw.line(screen, JAUNE, [40 + (i*4*timeSca), 400-dataGen[i]],[40 + ((i+1)*4*timeSca), 400-dataGen[i+1]],4)
                pygame.draw.line(screen, ORANGE, [40 + (i*4*timeSca), 400-data[i]],[40 + ((i+1)*4*timeSca), 400-data[i+1]],4)
                                
        pygame.draw.line(screen, GREEN, [40, 400],[500, 400],1)
        pygame.draw.line(screen, GREEN, [40, 400],[40, 145],1)
        pygame.draw.line(screen, GRIS,  [40+100, 400],[40+100, 145],2)
        pygame.draw.line(screen, GRIS,  [40+200, 400],[40+200, 145],2)
        pygame.draw.line(screen, GRIS,  [40+300, 400],[40+300, 145],2)
        pygame.draw.line(screen, GRIS,  [40+400, 400],[40+400, 145],2)
        pygame.draw.line(screen, GRIS2,  [40+50, 400],[40+50, 145],1)
        pygame.draw.line(screen, GRIS2,  [40+150, 400],[40+150, 145],1)
        pygame.draw.line(screen, GRIS2,  [40+250, 400],[40+250, 145],1)
        pygame.draw.line(screen, GRIS2,  [40+350, 400],[40+350, 145],1)
        pygame.draw.line(screen, GRIS,  [40, 400-256],[450, 400-256],2)
        pygame.draw.line(screen, GRIS,  [40, 400-128],[450, 400-128],2)
        pygame.draw.line(screen, GRIS2,  [40, 400-205],[450, 400-205],1)
        pygame.draw.line(screen, GRIS2,  [40, 400-154],[450, 400-154],1)
        pygame.draw.line(screen, GRIS2,  [40, 400-102],[450, 400-102],1)
        pygame.draw.line(screen, GRIS2,  [40, 400-51],[450, 400-51],1)
        
        pygame.draw.line(screen, RED, [40, 400-moyenne],[500, 400-moyenne],1)
        
        label_5V = myfont.render('5.0V', False, JAUNE)
        #label_3_3V = myfont.render('3.3V', False, JAUNE)
        label_2_5V = myfont.render('2.5V', False, JAUNE)
        label_0V = myfont.render('0.0V', False, JAUNE)
        
        label_time0 = myfont.render('0' + timeUnit, False, JAUNE)
        label_time25 = myfont.render("{:.2f}".format(timeBaseF*25/timeSca) + timeUnit, False, JAUNE)
        label_time50 = myfont.render("{:.2f}".format(timeBaseF*50/timeSca) + timeUnit, False, JAUNE)
        label_time75 = myfont.render("{:.2f}".format(timeBaseF*75/timeSca) + timeUnit, False, JAUNE)
        label_time100 = myfont.render("{:.2f}".format(timeBaseF*100/timeSca) + timeUnit, False, JAUNE)
        
        moyenneV = round((moyenne/255)*5,2)
        label_moyenneV = myfont.render(str(moyenneV) + 'V', False, VERT)
        screen.blit(label_moyenneV,(510,393-moyenne))
        
        minV = round((valeur_Min/255)*5,2)
        maxV = round((valeur_Max/255)*5,2)
        pointe = round(((valeur_Max-valeur_Min)/255)*5,2)
        label_min = myfont.render(str(minV) + 'V', False, WHITE)
        label_max = myfont.render(str(maxV) + 'V', False, TOMATE)
        label_pap = myfont.render('Vpp = ' + str(pointe) + 'V', False, VERT)
        pygame.draw.line(screen, WHITE, [40, 400-valeur_Min],[500, 400-valeur_Min],1)
        pygame.draw.line(screen, TOMATE, [40, 400-valeur_Max],[500, 400-valeur_Max],1)
        screen.blit(label_min,(510,393-valeur_Min+10))
        screen.blit(label_pap,(490,393-valeur_Min+30))
        screen.blit(label_max,(510,393-valeur_Max-10))
        
        screen.blit(label_5V, (5,138))
        #screen.blit(label_3_3V, (5,225))
        screen.blit(label_2_5V, (5,266))
        screen.blit(label_0V, (5,393))
        
        screen.blit(label_time0, (40,410))
        screen.blit(label_time25, (40+80,410))
        screen.blit(label_time50, (40+180,410))
        screen.blit(label_time75, (40+280,410))
        screen.blit(label_time100, (40+380,410))
        
        pygame.draw.rect(screen,BLUE,(237,30,80,22))
        if (onde==0):
            label_onde = myfont.render('Sinusoïde', False, JAUNE)
        else:
            if (onde==1):
                label_onde = myfont.render('Carrée', False, JAUNE)
            else:
                label_onde = myfont.render('Triangulaire', False, JAUNE)
        screen.blit(label_onde, (240,32))
        label_titreO = myfont.render("Forme d'onde :", False, JAUNE)
        screen.blit(label_titreO, (95,32))
        
        pygame.draw.rect(screen,BLUE,(237,60,80,22))
        label_ampli = myfont.render(str(2*ampli) + ' V', False, JAUNE)
        screen.blit(label_ampli,(240,62))
        label_titreA = myfont.render("Amplitude (p-à-p) :", False, JAUNE)
        screen.blit(label_titreA, (95,62))
        
        pygame.draw.rect(screen,BLUE,(237,90,80,22))
        label_ampli = myfont.render(str(freq) + ' Hz', False, JAUNE)
        screen.blit(label_ampli,(240,92))
        label_titreA = myfont.render("Fréquence :", False, JAUNE)
        screen.blit(label_titreA, (95,92))
        
        label_titreGen = myfont.render("GÉNÉRATEUR DE FONCTION", False, WHITE)
        screen.blit(label_titreGen, (95,4))
 
        pygame.draw.rect(screen,BLUE,(237,440,80,22))
        label_ampli = myfont.render(str(timeSca) + ' X', False, JAUNE)
        screen.blit(label_ampli,(240,442))
        label_titreA = myfont.render("Zoom (temps) :", False, JAUNE)
        screen.blit(label_titreA, (95,442))
        
        pygame.draw.rect(screen,BLUE,(237,470,80,22))
        label_ampli = myfont.render(str(N+1) + ' pts', False, JAUNE)
        screen.blit(label_ampli,(240,472))
        label_titreA = myfont.render("Nombre de points :", False, JAUNE)
        screen.blit(label_titreA, (95,472))
        
        pygame.display.flip()
            