import collections

from pandas.core.frame import DataFrame
import PlotterData
import pandas as pd
import numpy as np
import scipy.stats
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1.inset_locator import zoomed_inset_axes
from mpl_toolkits.axes_grid1.inset_locator import inset_axes
from matplotlib.lines import Line2D
from mpl_toolkits.axes_grid1.inset_locator import mark_inset
import seaborn as sns

import copy
import inspect
PlotterData.Initialize()
globalData = PlotterData.data
globalPathData = PlotterData.gpathdata


#transform data
#globalData["IncorectArea"]=globalData["IncorectArea"].apply(lambda x: x*100)

globalData["GenerationTime"]=globalData["GenerationTime"].apply(lambda x: x/1000)


AllData = PlotterData.GetDataInfo()
MazeTetMesh_50 = AllData["TetMesh"]["Maze"]["50"]
MazeTetMesh_100 = AllData["TetMesh"]["Maze"]["100"]
MazeTetMesh_200 = AllData["TetMesh"]["Maze"]["200"]
MazeSVO_50 = AllData["SVO"]["Maze"]["50"]
MazeSVO_100 = AllData["SVO"]["Maze"]["100"]
MazeSVO_200 = AllData["SVO"]["Maze"]["200"]

AsteroidTetMesh_50 = AllData["TetMesh"]["AsteroidField"]["50"]
AsteroidTetMesh_100 = AllData["TetMesh"]["AsteroidField"]["100"]
AsteroidTetMesh_200 = AllData["TetMesh"]["AsteroidField"]["200"]
AsteroidSVO_50 = AllData["SVO"]["AsteroidField"]["50"]
AsteroidSVO_100 = AllData["SVO"]["AsteroidField"]["100"]
AsteroidSVO_200 = AllData["SVO"]["AsteroidField"]["200"]

def ComplexityToVoxelSize(complexity):
    return 12800/ round((100*complexity+2))


def FilterFailedStructures(plotframes):
    result = []
    for plotframe in plotframes:
        plotframecopy = copy.copy(plotframe)
        plotframecopy.data = plotframe.data.loc[plotframe.data["OpenFreeCovered"] > 0.01]
        result.append(plotframecopy)
    return result

def CalulateRatio(xName: str, yName:str, plotFrame1,plotFrame2):
    selection1 = plotFrame1.data[[xName,yName]].loc[plotFrame1.data["OpenFreeCovered"] > 0.01]
    selection2 = plotFrame2.data[[xName,yName]].loc[plotFrame2.data["OpenFreeCovered"] > 0.01]

    bin1 = scipy.stats.binned_statistic(selection1[xName], selection1[yName], bins =50, range = (0.0,1.0)).statistic
    bin2 = scipy.stats.binned_statistic(selection2[xName], selection2[yName], bins =50, range = (0.0,1.0)).statistic

    def IsValid(x): return ~np.isnan(x) and x != 0

    result = []
    for i in range(0,len(bin1)):
        if IsValid(bin2[i]) and IsValid(bin1[i]): result.append(bin1[i]/bin2[i])


    return result
 



def PlotScatter(ax, xName: str, yName:str, plotFrames, attributename = "data"):
    for plotFrame in plotFrames:
        data = getattr(plotFrame, attributename)
        ax.scatter(data[xName], data[yName], label=plotFrame.name, edgecolors = "none", marker=plotFrame.markerstyle,facecolors= plotFrame.color,s=10)  # Plot some data on the axes.


def PlotMedian(ax,xName: str, yName:str, plotFrames, attributename = "data", numberBins = 20):
     for plotFrame in plotFrames:
         data = plotFrame if attributename == "" else getattr(plotFrame, attributename)
         selection = data[[xName,yName]]
         group = selection.groupby(pd.cut(selection[xName], numberBins))
         name = plotFrame.voxelsize + "_" + plotFrame.structure
         ax.plot(group.mean()[xName],group.median()[yName],label=name, color = plotFrame.color, linestyle = plotFrame.linestyle)


def PlotAverage(ax,xName: str, yName:str, plotFrames, attributename = "data",numberBins = 20):    
    for plotFrame in plotFrames:
        data = getattr(plotFrame, attributename)
        selection = data[[xName,yName]]
        group = selection.groupby(pd.cut(selection[xName], numberBins))
        name = plotFrame.voxelsize + "_" + plotFrame.structure
        ax.plot(group.mean()[xName],group.mean()[yName],label=name, color = plotFrame.color, linestyle = plotFrame.linestyle)

def PlotMinMax(ax,xName: str, yName:str, plotFrames, attributename = "data",numberBins = 20):
    for plotFrame in plotFrames:
        data = getattr(plotFrame, attributename)
        selection = data[[xName,yName]]
        maxx = selection[xName].max()
        group = selection.groupby(pd.cut(selection[xName], np.arange(selection[xName].min(),maxx+0.01,maxx/numberBins)))
        ax.fill_between(group.mean()[xName],group.max()[yName],group.min()[yName], alpha = 0.1, color = plotFrame.color)
        ax.plot(group.mean()[xName],group.mean()[yName],label=plotFrame.name, color = plotFrame.color)


def DrawBetterArrow(ax: plt.Axes, upArrow: bool):
    arrowStyle = "<-" if upArrow == True else "->"
    text = "Higher is better" if upArrow == True else "Lower is better"
    ax.annotate("", xy=(1.1,0.1), xycoords="axes fraction",xytext=(1.1,0.5), arrowprops=dict(arrowstyle=arrowStyle,color='black', linewidth =1.5,mutation_scale=30))
    ax.annotate(text,  xy=(1.14,0), xycoords="axes fraction",xytext=(1.14,0.30), rotation=90, va="center")

def FinalizeFigure(fig, lines, labels, saveName, title):
    plt.suptitle(title)
    fig.legend(lines, labels)
    fig.savefig("Figures/"+saveName+".png")
    plt.close(fig)

def cm_to_inch(value):
    return value/2.54



def CreateFreeSpacePlot():

    fig,ax = plt.subplots(1,2, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))
    PlotMedian(ax[0],"Scene Complexity", "OpenFreeCovered",  [MazeTetMesh_200,MazeSVO_200,MazeTetMesh_100,MazeSVO_100,MazeTetMesh_50,MazeSVO_50])
    PlotMedian(ax[1],"Scene Complexity", "OpenFreeCovered",  [AsteroidTetMesh_200,AsteroidSVO_200,AsteroidTetMesh_100,AsteroidSVO_100,AsteroidTetMesh_50,AsteroidSVO_50])

    ax[0].set_ylabel("Open Space Covered(%)")
    ax[0].set_xlabel("Scene Complexity")
    ax[0].title.set_text("Maze")
    ax[1].title.set_text("Asteroid")

    ax[0].set_ylim(0,100)
    lines,labels = ax[0].get_legend_handles_labels()
    DrawBetterArrow(ax[1],True)
    FinalizeFigure(fig, lines,labels,"FreeSpaceCovered", "Open Space Covered")

def CreateFreeSpaceFailedPlot():
    fig,ax = plt.subplots(1,2, sharey =True,figsize=(cm_to_inch(20),cm_to_inch(10)))

    def Plot(ax,plotframes):
        for plotframe in plotframes:
            data = plotframe.data
            data["Scene Complexity Groups"] = pd.qcut(data["Scene Complexity"],40)
            data = data.loc[data["OpenFreeCovered"] < 0.01]
            val = data["Scene Complexity Groups"].value_counts().apply(lambda x: x*4)
            name = plotframe.voxelsize + "_" + plotframe.structure
            ax.plot(np.linspace(1,0,40),val,label=name, color = plotframe.color, linestyle = plotframe.linestyle)

    Plot(ax[0], [MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200,MazeSVO_50,MazeSVO_100,MazeSVO_200])
    Plot(ax[1],[AsteroidTetMesh_50,AsteroidTetMesh_100,AsteroidTetMesh_200,AsteroidSVO_50,AsteroidSVO_100,AsteroidSVO_200])

    ax[0].title.set_text("Maze")
    ax[1].title.set_text("Asteroid")
    ax[0].set_ylabel("Fail probablity(%)")
    ax[0].set_xlabel("Scene complexity")
    lines,labels = ax[0].get_legend_handles_labels()
    DrawBetterArrow(ax[1],False)
    FinalizeFigure(fig, lines,labels,"FailedStructures", "Structure fail probablity")

    #print(struct1.head())




def CreateIncorrectAreaPlot(scatter:bool):

    #else:
     #   PlotMedian(ax[0],"Scene Complexity", "IncorectArea",  [MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200])
    #    PlotMedian(ax[1],"Scene Complexity", "IncorectArea",  [AsteroidTetMesh_50,AsteroidTetMesh_100,AsteroidTetMesh_200])

    def Plot(ax,plotframes):
        for plotframe in plotframes:
            xdata = plotframe.data.loc[plotframe.data["OpenFreeCovered"] > 0.01]
            name = plotframe.voxelsize + "_" + plotframe.structure
            group = xdata.groupby(pd.cut(xdata["Scene Complexity"], 20))
            ax.plot(group.mean()["Scene Complexity"], group.median()["IncorectArea"],label=name, color = plotframe.color, linestyle = plotframe.linestyle) 

    fig,ax = plt.subplots(1,2, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))
    if scatter:
        PlotScatter(ax[0],"Scene Complexity", "IncorectArea",  [MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200])
        PlotScatter(ax[1],"Scene Complexity", "IncorectArea",  [AsteroidTetMesh_50,AsteroidTetMesh_100,AsteroidTetMesh_200])
    else:
        Plot(ax[0], [MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200])
        Plot(ax[1], [AsteroidTetMesh_50,AsteroidTetMesh_100,AsteroidTetMesh_200])

    ax[0].set_ylabel("Incorrect Area(%)")
    ax[0].set_xlabel("Scene Complexity")
    ax[0].title.set_text("Maze")
    ax[1].title.set_text("Asteroid")
    ax[0].set_ylim(0,12)
    DrawBetterArrow(ax[1],False)
    lines,labels = ax[0].get_legend_handles_labels()

    FinalizeFigure(fig, lines,labels, "IncorrectArea", "Incorrect Area")

def CreateConnectionsPlot():
    fig,ax = plt.subplots(1,2, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))
    PlotMedian(ax[0],"Scene Complexity", "NumberOfConnections",  FilterFailedStructures([MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200,MazeSVO_50,MazeSVO_100,MazeSVO_200]))
    PlotMedian(ax[1],"Scene Complexity", "NumberOfConnections",  FilterFailedStructures([AsteroidTetMesh_50,AsteroidSVO_50,AsteroidTetMesh_100,AsteroidSVO_100,AsteroidTetMesh_200,AsteroidSVO_200]))

    ax[0].set_ylabel("Number Of Connections")
    ax[0].set_xlabel("Scene Complexity")
    ax[0].title.set_text("Maze")
    ax[1].title.set_text("Asteroid")
    ax[0].set_yscale('log')
    ax[1].set_yscale('log')

    lines,labels = ax[0].get_legend_handles_labels()
    DrawBetterArrow(ax[1],False)
    FinalizeFigure(fig, lines,labels,"NumberOfConnections", "Number Of Connections")

def CreateNodesPlot():
    def Filter(dataframe):
        return dataframe.loc[dataframe["OpenFreeCovered"] > 0.01]

    def Plot(ax,xName: str, yName:str, plotFrames):
        for plotFrame in plotFrames:
            filtereddata = Filter(plotFrame.data)
            selection = filtereddata[[xName,yName]]
            group = selection.groupby(pd.cut(selection[xName], 20))
            name = plotFrame.voxelsize + "_" + plotFrame.structure
            ax.plot(group.mean()[xName],group.median()[yName],label=name, color = plotFrame.color, linestyle = plotFrame.linestyle)

    fig,ax = plt.subplots(1,2, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))
    Plot(ax[0],"Scene Complexity", "NumberOfNodes", [MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200,MazeSVO_50,MazeSVO_100,MazeSVO_200])
    Plot(ax[1],"Scene Complexity", "NumberOfNodes", [AsteroidTetMesh_50,AsteroidSVO_50,AsteroidTetMesh_100,AsteroidSVO_100,AsteroidTetMesh_200,AsteroidSVO_200])
   # PlotMinMax(ax[0],"Scene Complexity", "NumberOfNodes",  [MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200,MazeSVO_50,MazeSVO_100,MazeSVO_200])
    #PlotMinMax(ax[1],"Scene Complexity", "NumberOfNodes",  [AsteroidTetMesh_50,AsteroidSVO_50,AsteroidTetMesh_100,AsteroidSVO_100,AsteroidTetMesh_200,AsteroidSVO_200])
    ax[0].set_yscale('log')
    ax[1].set_yscale('log')
    ax[0].set_ylabel("Number Of Nodes")
    ax[0].set_xlabel("Scene Complexity")
    ax[0].title.set_text("Maze")
    ax[1].title.set_text("Asteroid")
    lines,labels = ax[0].get_legend_handles_labels()
    DrawBetterArrow(ax[1],False)
    FinalizeFigure(fig, lines,labels,"NumberOfNodes", "Number Of Nodes")

def CreateNodesConnectionPlot():
    fig,ax = plt.subplots(1,2, sharey =True,figsize=(cm_to_inch(20),cm_to_inch(10)))
    PlotScatter(ax[0],"NumberOfConnections", "NumberOfNodes",  [MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200,MazeSVO_50,MazeSVO_100,MazeSVO_200])
    PlotScatter(ax[1],"NumberOfConnections", "NumberOfNodes",  [AsteroidTetMesh_50,AsteroidTetMesh_100,AsteroidTetMesh_200,AsteroidSVO_50,AsteroidSVO_100,AsteroidSVO_200])

    ax[0].set_ylabel("Number Of Nodes")
    ax[0].set_xlabel("Scene Complexity")
    ax[0].title.set_text("Maze")
    ax[1].title.set_text("Asteroid")


    lines,labels = ax[0].get_legend_handles_labels()

    FinalizeFigure(fig, lines,labels,"NodesConnections", "Nodes Connections")

def CreateGenerationTimePlot():
    def Filter(dataframes):
        result = []
        for dataframe in dataframes:
            c = copy.copy(dataframe)
            c.data = dataframe.data.loc[dataframe.data["OpenFreeCovered"] > 0.01]
            result.append(c)
        return result

    fig,ax = plt.subplots(2,3, sharey =False,figsize=(cm_to_inch(30),cm_to_inch(20)))
    #PlotMedian(ax[0],"Scene Complexity", "GenerationTime",  [MazeTetMesh_50,MazeSVO_50])
    #ScatterPlot(ax[1],"Scene Complexity", "GenerationTime",  [AsteroidTetMesh_50,AsteroidTetMesh_100,AsteroidTetMesh_200,AsteroidSVO_50,AsteroidSVO_100,AsteroidSVO_200])
    PlotMinMax(ax[0][0],"Scene Complexity", "GenerationTime", Filter([MazeTetMesh_50,MazeSVO_50]))
    PlotMinMax(ax[0][1],"Scene Complexity", "GenerationTime", Filter([MazeTetMesh_100,MazeSVO_100]))
    PlotMinMax(ax[0][2],"Scene Complexity", "GenerationTime", Filter([MazeTetMesh_200,MazeSVO_200]))

    PlotMinMax(ax[1][0],"Scene Complexity", "GenerationTime", Filter([AsteroidTetMesh_50,AsteroidSVO_50]))
    PlotMinMax(ax[1][1],"Scene Complexity", "GenerationTime", Filter([AsteroidTetMesh_100,AsteroidSVO_100]))
    PlotMinMax(ax[1][2],"Scene Complexity", "GenerationTime", Filter([AsteroidTetMesh_200,AsteroidSVO_200]))

    for x in range(0,2):
        for y in range(0,3):
            ax[x][y].set_xlim([0.0,1.0])

    ax[0][0].set_ylim([0.0,750])
    ax[0][0].title.set_text("Maze_50")
    ax[0][1].title.set_text("Maze_100")
    ax[0][2].title.set_text("Maze_200")

    ax[1][0].title.set_text("Asteroid_50")
    ax[1][1].title.set_text("Asteroid_100")
    ax[1][2].title.set_text("Asteroid_200")

    ax[1][0].set_ylabel("GenerationTime (s)")
    ax[1][0].set_xlabel("Scene Complexity")
    legend_elements = [Line2D([0], [0], color='r', lw=2, label='SVO'),
                   Line2D([0], [0], color='b', lw=2,label='TetMesh')]


    DrawBetterArrow(ax[1][2],False)
    plt.suptitle("Generation Time")
    fig.legend(handles=legend_elements)
    fig.savefig("Figures/"+"GenerationTime"+".png")
    plt.close(fig)

def CreateMemoryUsagePlot():
    fig,ax = plt.subplots(1,2, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))
    PlotMedian(ax[0],"Scene Complexity", "MemoryUsage",  FilterFailedStructures([MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200,MazeSVO_50,MazeSVO_100,MazeSVO_200]))
    PlotMedian(ax[1],"Scene Complexity", "MemoryUsage",  FilterFailedStructures([AsteroidTetMesh_50,AsteroidTetMesh_100,AsteroidTetMesh_200,AsteroidSVO_50,AsteroidSVO_100,AsteroidSVO_200]))


    ax[0].set_ylabel("MemoryUsage (MB)")
    ax[0].set_xlabel("Scene Complexity")
    ax[0].title.set_text("Maze")
    ax[1].title.set_text("Asteroid")
    #ax[0].set_yscale('log')
    #ax[1].set_yscale('log')


    lines,labels = ax[0].get_legend_handles_labels()
    DrawBetterArrow(ax[1],False)

    FinalizeFigure(fig, lines,labels,"MemoryUsage", "Memory Usage")




def CreateStatesPlot(v:int):
    if v > 2 | v < 0: return
    def CountPathStates2(pathData):
        succescount = [0.0] * 1001
        failedcount = [0.0] * 1001
        toolongcount = [0.0] * 1001

        for index, row in pathData[["TestID","Status"]].iterrows():
            state = row["Status"]
            ID = row["TestID"]%1000
            if ID == 0: ID = 1000
            if(state == "Success"): succescount[ID] += 1.0;
            if(state == "Failed"): failedcount[ID] += 1.0;
            if(state == "Took too long"): toolongcount[ID] += 1.0;

        return succescount,failedcount,toolongcount

    def Plot(ax,plotframes, bins = 20):

        def StatisticFilter(arr):
            return (sum(arr)/(50000/bins))*100

        for plotframe in plotframes:
            arr = CountPathStates2(plotframe.pathdata)[v]
            bin_means, bin_edges, binnumber =scipy.stats.binned_statistic(np.linspace(0.0,1.0, len(arr),True),arr, statistic = StatisticFilter, bins=bins, range=(0,1))
            bin_centres = np.add(bin_edges, 1.0/bins/2.0).tolist()
            name = plotframe.voxelsize + "_" + plotframe.structure
            ax.plot(bin_centres[0:20],bin_means,label=name, color = plotframe.color, linestyle = plotframe.linestyle)


    fig,ax = plt.subplots(1,2, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))
    #DrawBars(ax[0][0], MazeSVO_50.pathdata, "Maze_50")

    Plot(ax[0], [MazeTetMesh_50,MazeTetMesh_100,MazeTetMesh_200,MazeSVO_50,MazeSVO_100,MazeSVO_200])
    Plot(ax[1], [AsteroidTetMesh_50,AsteroidTetMesh_100,AsteroidTetMesh_200,AsteroidSVO_50,AsteroidSVO_100,AsteroidSVO_200])

    ax[0].set_ylabel(("Succes probability" if v == 0 else ("Failure probability" if v == 1 else "Too long probability")) + " (%)" )
    ax[0].set_xlabel("Scene Complexity")
    ax[0].title.set_text("Maze")
    ax[1].title.set_text("Asteroid")
    DrawBetterArrow(ax[1],v==0)
    lines,labels = ax[0].get_legend_handles_labels()
    if v == 2: FinalizeFigure(fig, lines,labels,"PathTooLong", "Path took too long probability")
    if v == 1: FinalizeFigure(fig, lines,labels,"PathFailure", "Path fail probability")
    if v == 0: FinalizeFigure(fig, lines,labels,"PathSucces", "Path succes probability")


def CountPathStates(pathData):
    succescount = [0] * 1001
    failedcount = [0] * 1001
    toolongcount = [0] * 1001

    for index, row in pathData[["TestID","Status"]].iterrows():
        state = row["Status"]
        ID = row["TestID"]%1000
        if ID == 0: ID = 1000
        if(state == "Success"): succescount[ID] += 1;
        if(state == "Failed"): failedcount[ID] += 1;
        if(state == "Took too long"): toolongcount[ID] += 1;

    return succescount,failedcount,toolongcount


def CreateSlicePlot(complexity):
    width = 0.025
    structure1 = MazeSVO_50
    structure2 = MazeTetMesh_50

    data1 = structure1.data
    data2 = structure2.data

    data1 = data1.loc[(data1["Scene Complexity"] >= (complexity - width)) & (data1["Scene Complexity"] <= (complexity + width))]
    data2 = data2.loc[(data2["Scene Complexity"] >= (complexity - width)) & (data2["Scene Complexity"] <= (complexity + width))]

    data1 = data1.loc[data1["OpenFreeCovered"] > 0.01]
    data2 = data2.loc[data2["OpenFreeCovered"] > 0.01]


    df1 = data1["OpenFreeCovered"]
    df2 = data2["OpenFreeCovered"]

    def DrawArrow(ax: plt.Axes, upArrow: bool):
        arrowStyle = "<-" if upArrow == True else "->"
        text = "Higher is better" if upArrow == True else "Lower is better"
        ax.annotate("",  xy=(1.12,0.1), xycoords="axes fraction",xytext=(1.12,0.3), arrowprops=dict(arrowstyle=arrowStyle,color='black', linewidth =1.5,mutation_scale=30))
        ax.annotate(text,  xy=(1.05,0.3), xycoords="axes fraction",xytext=(1.05,0.3), rotation=90)



    def Plot(axes,plotframe1, plotframe2, vars, names):
        for i,var in enumerate(vars):
            bplot = axes[i].boxplot([plotframe1[var].to_numpy(),plotframe2[var].to_numpy()], widths = 0.8, patch_artist=True, notch=True)
            axes[i].set_title(names[i],pad=15)
            axes[i].set_xticks([1], [''])
            DrawArrow(axes[i], i==0)
            bplot['boxes'][0].set_facecolor("red")
            bplot['boxes'][1].set_facecolor("blue")


    fig,ax = plt.subplots(1,5, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))

    fig.set_figheight(5)
    Plot(ax, data1,data2,["OpenFreeCovered","NumberOfNodes","NumberOfConnections","GenerationTime","MemoryUsage"],["Free space","Num nodes","Num connections","Gen time", "Mem usage"])

    legend_elements = [Line2D([0], [0], color='r', lw=2, label= str(structure1.voxelsize) + "_" + structure1.structure),
                   Line2D([0], [0], color='b', lw=2,label= str(structure2.voxelsize) + "_" + structure2.structure)]
    fig.legend(handles=legend_elements)
    plt.suptitle("Slice Maze (c="+ str(complexity) +", w="+ str(width*2) +")")
    fig.tight_layout()
    fig.savefig("Figures/"+"Slices"+".png")
    plt.close(fig)

def CreatePathTimePlot():
    fig,ax = plt.subplots(2,3, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))
    #PlotMinMax(ax[0],"Length of Path", "Generation Time",  [AsteroidTetMesh_200], "pathdata", numberBins = 20)
   # PlotMinMax(ax[1],"Length of Path", "Generation Time",  [AsteroidSVO_200], "pathdata", numberBins = 20)
    PlotMedian(ax[0][0], "PathID", "Generation Time", [AsteroidTetMesh_50,AsteroidSVO_50],"pathdata", numberBins = 20 )
    PlotMedian(ax[0][1], "PathID", "Generation Time", [AsteroidTetMesh_100,AsteroidSVO_100],"pathdata", numberBins = 20 )
    PlotMedian(ax[0][2], "PathID", "Generation Time", [AsteroidTetMesh_200,AsteroidSVO_200],"pathdata", numberBins = 20 )
    PlotMedian(ax[1][0], "PathID", "Generation Time", [MazeTetMesh_50,MazeSVO_50],"pathdata", numberBins = 20 )
    PlotMedian(ax[1][1], "PathID", "Generation Time", [MazeTetMesh_100,MazeSVO_100],"pathdata", numberBins = 20 )
    PlotMedian(ax[1][2], "PathID", "Generation Time", [MazeTetMesh_200,MazeSVO_200],"pathdata", numberBins = 20 )

    plt.suptitle("Generation Time")

    legend_elements = [Line2D([0], [0], color='r', lw=2, label='SVO'),
                   Line2D([0], [0], color='b', lw=2,label='TetMesh')]

    fig.legend(handles = legend_elements)
    fig.tight_layout()
    fig.savefig("Figures/"+"PathTime"+".png")
    plt.close(fig)


def TestPlot(data1, data2):

    counter1 = 0
    counter2 = 0
    result = []
    for n in range(0,len(data1.index)):
        row1 = data1.iloc[n]
        row2 = data2.iloc[n]
        if row1["Status"] != "Success" or row2["Status"] != "Success": continue
        #result.append(row1["Length of Path"]-row2["Length of Path"])
        if row1["Length of Path"] > row2["Length of Path"]: counter1+=1
        else: counter2 +=1
    print(counter1+counter2)
    print(counter1/(counter1+counter2)*100)
    #fig,ax = plt.subplots(1,2, sharey =False,figsize=(cm_to_inch(20),cm_to_inch(10)))

    #ax[0].scatter(range(0,len(result)), result)
    #fig.savefig("Figures/"+"Test"+".png")
    #plt.close(fig)
#CreateFreeSpacePlot()
print("Start analysing")
#CreateNodesConnectionPlot()
#CreateSlicePlot(0.8)
#TestPlot(AsteroidTetMesh_50.pathdata, AsteroidSVO_50.pathdata)
#TestPlot(AsteroidTetMesh_100.pathdata, AsteroidSVO_100.pathdata)
#TestPlot(AsteroidTetMesh_200.pathdata, AsteroidSVO_200.pathdata)
CreateFreeSpacePlot()
#ratiolist = CalulateRatio("Scene Complexity", "MemoryUsage", MazeTetMesh_50,MazeSVO_50 )
#minratio = max(ratiolist)
#print(minratio)


