def dfs(node, visited,graph):
   
    for neighbor in graph[node]:
        if neighbor not in visited:
            retGraph.append([node,neighbor])
            visited.add(neighbor)
            dfs(neighbor,visited,retGraph)
        if neighbor in visited:
            add = [node,neighbor]
            if add not in retGraph:
                retGraph.append([node,neighbor])


graph = {1:[2,3],2:[1,3],3:[1,2]}
dfs(1,set(),graph)