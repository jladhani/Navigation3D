import matplotlib.pyplot as plt
import pandas as pd
from collections import namedtuple
from numpy import arange


Data = pd.read_csv("Data.csv",sep=';',decimal=',')

PlotTuple = namedtuple("PlotTuple", ["SVO", "TetMesh"])

def CreatePlotTuple(columName: str):
    return PlotTuple([float(i) for i in SVOList[columName]], [float(i) for i in TetMeshList[columName]])

def ExtractData(Dict, columName: str):
    return [float(i) for i in Dict[columName]]

SVOList = Data.loc[Data['Type'] == 'SVO']
TetMeshList = Data.loc[Data['Type'] == 'TetMesh']

x = list(arange(0.0, 1.0, 1.0/len(SVOList['ID'])))

fullArray = list(TetMeshList["ID"])
missingNumbers = list()

testm = TetMeshList["NumberOfNodes"].values
heights = testm.mean()
print(heights)

def PlotMeasurement(measurementName: str, axisName: str ="", yTicks = []):
    MeasurementTuple = CreatePlotTuple(measurementName)



    fig,ax = plt.subplots()
    c = '#0000ff'
    ax.scatter(x, MeasurementTuple.SVO, label='SVO_100', color=c)  # Plot some data on the axes.
    c = '#FF7F50'
    ax.scatter(x, MeasurementTuple.TetMesh, label='TetMesh_100', color=c)  # Plot some data on the axes.

    ax.set_xticks([0,0.5,1])
    if len(yTicks) > 0:
        ax.set_yticks(yTicks)
    ax.set_xlabel('Complexity')
    ax.set_ylabel(measurementName if axisName == "" else axisName)
    ax.legend();
    nam = "Figures/"+measurementName+".png"
    fig.savefig(nam)
    plt.close(fig)

PlotMeasurement("OpenFreeCovered", "Free Space Covered", [0,25,50,75,100])
PlotMeasurement("IncorectArea")
PlotMeasurement("NumberOfConnections")
PlotMeasurement("NumberOfNodes")
PlotMeasurement("GenerationTime")
PlotMeasurement("MemoryUsage", "Memory Usage (MB)")

Data150 = pd.read_csv("Data150.csv",sep=';',decimal='.')
Data100 = pd.read_csv("Data100.csv",sep=';',decimal='.')
Data50 = pd.read_csv("Data50.csv",sep=';',decimal='.')

SVOData = {
    '50': Data50.loc[Data50['Type'] == 'SVO'],
    '100': Data100.loc[Data100['Type'] == 'SVO'],
    '150': Data150.loc[Data150['Type'] == 'SVO']
    }
TetMeshData = {
    '50': Data50.loc[Data50['Type'] == 'TetMesh'],
    '100': Data100.loc[Data100['Type'] == 'TetMesh'],
    '150': Data150.loc[Data150['Type'] == 'TetMesh']
    }

def MultiGraphFreeSpace(): 
    OpenSpaceCoveredSVO100 = ExtractData(SVOData['100'],"OpenFreeCovered")
    OpenSpaceCoveredTetmesh50 = ExtractData(TetMeshData['50'],"OpenFreeCovered")
    OpenSpaceCoveredTetmesh100 = ExtractData(TetMeshData['100'],"OpenFreeCovered")

    fig,ax = plt.subplots()

    c = '#FFA500'
    ax.plot(x, OpenSpaceCoveredTetmesh50, label='TetMesh_50', color=c)  # Plot some data on the axes.
    c = '#FF7F50'
    ax.plot(x, OpenSpaceCoveredTetmesh100, label='TetMesh_100', color=c)  # Plot some data on the axes.
    c = '#0000ff'
    ax.plot(x, OpenSpaceCoveredSVO100, label='SVO_100', color=c)  # Plot some data on the axes.

    ax.set_xticks([0,0.5,1])
    ax.set_yticks([0,25,50,75,100])

    ax.set_xlabel('Complexity')
    ax.set_ylabel("Free Space Covered")
    ax.legend();
    nam = "Figures/"+"FreeSpaceCombined"+".png"
    fig.savefig(nam)
    plt.close(fig)

def MultiGraphNumberOfNodes():
    NumberOfNodesSVO100 = ExtractData(SVOData['100'],"NumberOfNodes")
    NumberOfNodesTetmesh50 = ExtractData(TetMeshData['50'],"NumberOfNodes")
    NumberOfNodesTetmesh100 = ExtractData(TetMeshData['100'],"NumberOfNodes")
    fig,ax = plt.subplots()

    c = '#FFA500'
    ax.plot(x, NumberOfNodesTetmesh50, label='TetMesh_50', color=c)  # Plot some data on the axes.
    c = '#FF7F50'
    ax.plot(x, NumberOfNodesTetmesh100, label='TetMesh_100', color=c)  # Plot some data on the axes.
    c = '#0000ff'
    ax.plot(x, NumberOfNodesSVO100, label='SVO_100', color=c)  # Plot some data on the axes.

    ax.set_xticks([0,0.5,1])

    ax.set_xlabel('Complexity')
    ax.set_ylabel("NumberOfNodes")
    ax.legend();
    nam = "Figures/"+"NumberOfNodesCombined"+".png"
    fig.savefig(nam)
    plt.close(fig)

MultiGraphFreeSpace()
MultiGraphNumberOfNodes()