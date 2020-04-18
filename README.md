# FORAGE HARVEST
zed相机计算出的距离是目标离相机左眼的距离  
1. 已知相交彩图上四个角点的坐标。  
2. 通过深度图或者点云图获得z值，此处z值就是相机距离四个角点的距离。  
3. 计算落点的z值，若小于某个范围，则高度已达到。  



## 落点的满溢度检测
- 对落点设置了一个结构体，属性1：它周边一定范围内的所有点，属性2：距离值  
1. 根据彩图上四个角点的坐标，在中轴线上选取六个落点  
2. 根据像素坐标获取每个点（四个角点、六个落点）点云图上的坐标（以下用的都是点云图的坐标）
3. 根据四个角点的坐标计算出平面方程  
4. 从车尾到车头依次计算每个落点与平面的距离（计算出落点周边一定范围内所有点到平面的距离，再取平均值）   
- 若为负，则为绿色，表示未喷满  
- 若为正且大于某个值，则为红色，表示喷满  
- 将满未满，则为黄色  
5. 参考：[https://wenku.baidu.com/view/a99d28a70508763231121296.html]  
- 此处有一个问题：根据像素坐标获取点云图上的坐标，万一刚好这个位置点云图上没有点呢？  
选取落点周边的一片范围，累加所有在这个范围内点离平面的距离，再求平均值  
- 四个角点不存在这个问题，因为是从黄色像素部分取的，只需考虑落点？  
- 决定不考虑只有两条边的情况（两条边时属于未检测到车筐）  
- 方法二：检测6个落点改为检测车斗边缘的12个点（如图中黄色的点）  
![黄色的点为要检测的点](/home/elsa/CLionProjects/forageHarvest/111.png)
1. 若落点对应上下两个边缘点均大于0，则落点为红色表示满  
2. 若有一个边缘点未满，则为绿色，可继续喷洒（防止饲料遮挡的情况）  
## 运动规划
#### 从右到左
1. 从最右的落点（第一个）开始喷洒
2. 若当前落点x（二维）位于下落柱范围之间   
- 落点没满，则不指示箭头，继续喷洒  
- 落点满了，则指示左箭头（向左移），到达下一个落点  
3. 若当前落点x（二维）位于下落柱范围右边   
- 落点没满，则指示右箭头，移动下落柱到当前落点  
- 落点满了，则指示左箭头（向左移），到达下一个落点  
4. 若当前落点x（二维）位于下落柱范围左边   
- 落点没满，则指示左箭头，移动下落柱到当前落点  
- 落点满了，则指示左箭头（向左移），移动下落柱到达下一个落点  
5. 落点满没满用三维落点的距离值，落点在不在下落柱的范围之间用二维落点的x值  


## 其他问题
- 深度图的z值和点云图的xyz什么关系？  
