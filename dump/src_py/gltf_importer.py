import json
from pygltflib import GLTF2
from dataclasses import dataclass

import sys, struct, os
if sys.stdin is not None:
    stdOut = open(sys.stdout.fileno(), 'w+b')
    stdErr = open(sys.stderr.fileno(), 'w')

    filePath =os.getcwd() +  input()#  './resourceFiles/models/bunny/scene.gltf'#  
    floatsPerVertex = {
        'SCALAR':1,
        'VEC2':2,
        'VEC3':3,
        'VEC4':4
    }
    componentTypeDict = {
        5126 : 'f', #float
        5125 : 'I', #uInt
        5123: 'H',  #uShort
        5122: 'h'   #short
    }
    typeBytelength = {
        5126 : 4,
        5125 : 4,
        5123: 2,
        5122: 2
    }

    @dataclass
    class Mesh:
        positions:list
        normals:list
        texcoord:list
        indices:list
        indicesType:str
        texturePath:str
        rotation:list = None
        translation:list = None
        vertices: list = None
        def __post_init__(self):
            self.vertices = []
            for pos,normal,texCoord in zip(self.positions, self.normals, self.texcoord):
                vertexList = []#current vertex as a list
                vertexList.extend(pos)
                vertexList.extend((1.0,1.0,1.0))#color
                vertexList.extend(texCoord)
                vertexList.extend(normal)
                self.vertices.append(vertexList)
    #prepare the JSON
    bunnyFileObject = GLTF2().load(filePath)
    modelJSON = json.loads(bunnyFileObject.gltf_to_json())


    def getNumbers(accessor:dict, data:bytes) -> list:
        #get accessor values
        bufferViewIndex = accessor.setdefault('bufferView',1)
        count = accessor['count']
        accesorType = accessor['type']
        componentType = accessor['componentType']
        accByteOffset = accessor.setdefault('byteOffset',0)#just in case the accessor has an offset

        #use bufferView to know how to read
        bufferIndex = modelJSON['bufferViews'][bufferViewIndex]['buffer']
        generalByteOffset = modelJSON['bufferViews'][bufferViewIndex]['byteOffset']
        whereItStarts = generalByteOffset + accByteOffset

        #read the data and pack it into allData
        allData = []
        if accesorType in ['VEC3', 'VEC2']:
            for i in range(count):
                vector = struct.unpack(
                    f'{floatsPerVertex[accesorType]}{componentTypeDict[componentType]}',
                    data[whereItStarts + (i * typeBytelength[componentType] * floatsPerVertex[accesorType]) :whereItStarts  + ((i + 1) *  typeBytelength[componentType] * floatsPerVertex[accesorType])])
                allData.append(vector)
        else:
            for i in range(count):
                allData.extend(struct.unpack(
                    f'{floatsPerVertex[accesorType]}{componentTypeDict[componentType]}',
                    data[whereItStarts + (i * typeBytelength[componentType] * floatsPerVertex[accesorType]) :whereItStarts  + ((i + 1) *  typeBytelength[componentType] * floatsPerVertex[accesorType])]))
        return allData

    def main():
        #get the binary data path
        pathToBin = os.getcwd() + '\\resourceFiles\\models\\' + modelJSON['meshes'][0]['name'] + '\\'+ modelJSON['buffers'][0]['uri']

        allMeshes = []
        #get the data from all meshes
        with open(pathToBin, 'rb') as binFile:
            binData = binFile.read()

            #returns how many meshes to expect and awaits
            #stdErr.write("\npython returns amount of meshes to expect\n")
            stdOut.write(struct.pack("I", len(allMeshes)))
            #stdErr.write("python done returning amount of meshes\n")
            #stdErr.write("\npython done returning amount of meshes, awaiting green flag\n")

            #stdErr.write("\npython continues\n")

            for i,meshWithOtherStuff in enumerate(modelJSON['meshes']):#other stuff = otros primitivos y el nombre de la mesh
                mesh = meshWithOtherStuff['primitives'][0]
                #get indices from mesh
                indicesAccIndex = mesh['indices']
                materialAccIndex = mesh['material']
                posAccIndex = mesh['attributes']['POSITION']
                normalAccIndex = mesh['attributes']['NORMAL']
                texcoordAccIndex = mesh['attributes']['TEXCOORD_0']
                
               
                #get vertex and EBO data
                indices = getNumbers(modelJSON['accessors'][indicesAccIndex], binData)
                position = getNumbers(modelJSON['accessors'][posAccIndex], binData)
                normals = getNumbers(modelJSON['accessors'][normalAccIndex], binData)
                texCoords = getNumbers(modelJSON['accessors'][texcoordAccIndex], binData)

                #get image stuff
                imageIndex = modelJSON['textures'][materialAccIndex]['source']
                imagePath = os.getcwd() + 'resourceFiles\\models\\' + modelJSON['images'][imageIndex]['uri']

                allMeshes.append(Mesh(position, normals,texCoords, indices,componentTypeDict[modelJSON['accessors'][indicesAccIndex]['componentType']] ,imagePath))
            for node in modelJSON['nodes']:
                meshIndex = node['mesh']
                allMeshes[meshIndex].rotation = node['rotation']
                allMeshes[meshIndex].translation = node['translation']

        #return all data
        for currMesh in allMeshes:

            #write the vertices
            buffer = bytearray()
            buffer.extend(struct.pack('I',len(currMesh.vertices)))#amount
            for vertex in currMesh.vertices:                       #data
                buffer.extend(struct.pack('11f',*vertex))
            #print(allMeshes[0].vertices[0])  this vertex has data = {-0.03783. 0.004475, -0.119358}
            stdOut.write(buffer)

            #write indices

            buffer.clear()
            buffer.extend(struct.pack('I',len(currMesh.indices)))
            buffer.extend(struct.pack(f'{len(currMesh.indices)}I',*currMesh.indices))

            stdOut.write(buffer)

            #send posData
            buffer.clear()
            buffer.extend(struct.pack('4f',*currMesh.rotation))
            buffer.extend(struct.pack('3f', *currMesh.translation))

            #stdErr.write('\npython sending meshPosData\n')
            stdOut.write(buffer)
            #stdErr.write('\npython done sending\n')

        #end
        stdErr.write('\npython ending\n')
        sys.stdin.close()
        sys.stdout.close()

        sys.exit(0)
else:
    print(1)
    sys.stdin.close()
    sys.stdout.close()

    sys.exit(1)

if __name__ == '__main__':
    main()
    
#TODO the stuff below to try and make the vertices
"""
    for each index of modelJSON['meshes']
        i go to its ['primitives']
            fetch its indicesAccIndex from ['indices'] 
            fetch its materialAccIndex from ['material']
            go to its ['attributes']
                fetch its AccInd from 'POSITION','NORMAL','TEXCOORD_0'
        
        go to modelJSON['accessors']
            get how to read their pos,Normal,textCoord and indexOrder with the accIndex
                i do this getting 'count', 'bufferView', 'componentType', and 'type'
        
        use bufferViewIndex to get 'buffer', 'byteLength'?, 'byteOffset' to know how to read

        DATA: read the binary data from the bin file using the accessor data
            packed indices, position, normal, texcoord_0
        go to modelJSON['textures'] use materialAccIndex to fetch imageIndex from ['source']

        go to modelJSON['images'] use imageIndex to fetch textureUri from ['uri']

        DATA: use textureUri to know the texture to use
            one per mesh
    Now i loop through modelJSON['nodes']
        i check its ['mesh'] to fetch the index of the mesh this applies to
        fetch the rotationQuaternion from ['rotation']
        get the translation vector from ['translation']    

    DATA: associate each rotation and translation with a mesh
        one of each per mesh


    Now each mesh has an associated 
        for vertex:
            -packed pos list
            -packed normal list
            -packed texcoord list

        for EBO:
            -packed indices list
        
        for mesh:
            -rotation quaternion
            -translation vector
            -texture path

    build the vertices with the packed lists
"""
#TODO try cython to turn this into a c function that i can call
"""
-might be using pyinstaller with extra steps
"""


#Try
"""
try: make the python process send data 3 times
-one to get the vertices to make the vertices list
-one to get the indices list
-one to get mesh data
it waits to send each one using stdin.read()

after that build the meshes
"""

#add
"""
To make it more complete i need to
-get wich buffer to read from using modelJSON['buffers']
-see if the anything isnt always included
-take into account multiple primitives indices instead of only using the first
-make the c side support data types other than vec3 and vec2
-make thw writing data support multiple meshes
"""


###TEST##
##index = 0
"""print(struct.pack(f'1I',allMeshes[0].indices[index]))
print(f'int from bytes:{int.from_bytes(struct.pack(f">1I",allMeshes[0].indices[index]))}')
print(f'int from list:{allMeshes[0].indices[index]}')
print("")
print(f'int from bytes in bits:{bin(int.from_bytes(struct.pack(f">1I",allMeshes[0].indices[index])))}')
print(f'int from list in bits:{bin(allMeshes[0].indices[index])}')
print(f'c number to bin: {bin(3179354655)}')"""
#time.sleep(7)