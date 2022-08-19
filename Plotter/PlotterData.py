from typing import Dict
import matplotlib.pyplot as plt

import pandas as pd
from pandas.core.frame import DataFrame



def Initialize():
    global data
    global gpathdata
    data = pd.read_csv("Data.csv",sep=';',decimal=',')
    gpathdata = pd.read_csv("DataPaths.csv",sep=';',decimal=',',index_col=False)


class PlotDataFrame():
	pass

def GetDataFrame(VoxelSize:int, Structure:str, Scene:str):
    obj = PlotDataFrame()
    obj.data = data.loc[(data["Voxel Size"]==VoxelSize) & (data["Type"]==Structure) & (data["WorldType"]== Scene)]
    obj.pathdata = gpathdata.loc[(gpathdata["Voxel Size"]==VoxelSize) & (gpathdata["Type"]==Structure) & (gpathdata["WorldType"]== Scene)]
    obj.name = Structure + "_" + Scene + "_" + str(VoxelSize)
    obj.structure = Structure
    obj.scene = Scene
    obj.voxelsize = str(VoxelSize)

    if Structure == "SVO":
        obj.color = "#ff0000"
    if Structure == "TetMesh":
        obj.color = "#0000ff"

    if VoxelSize == 50:
        obj.linestyle = "-"
        obj.markerstyle = "o"
    if VoxelSize == 100:
        obj.linestyle = "--"
        obj.markerstyle = "+"
    if VoxelSize == 200:
        obj.linestyle = ":"
        obj.markerstyle = "*"



    return obj


def GetDataInfo():

    Structures = ["SVO", "TetMesh"]
    Scenes = ["Maze", "AsteroidField"]
    VoxelSizes = [50,100,200]

    newDict = {}

    for structure in Structures:
        newDict[structure] = {}
        for scene in Scenes:
            newDict[structure][scene] = {}
            for voxelSize in VoxelSizes:
                newDict[structure][scene][str(voxelSize)] = GetDataFrame(voxelSize,structure,scene)
    

    return newDict