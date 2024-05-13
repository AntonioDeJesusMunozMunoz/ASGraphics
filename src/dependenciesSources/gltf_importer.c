#include <gltf_importer.h>

#define INDEX 208351 //la cantidad de vertices que dice en el gltf -2

/*builders*/
/*
void createMeshWithBuffers(Mesh *m, indicesBuffer *indices, vertexBuffer *vertices, 
MeshPosDataBuffer *meshPosData, GLuint vao, VBO *vbo){

    glBindVertexArray(vao);
    //give vbo data
    glBindBuffer(GL_ARRAY_BUFFER, vbo->handle);
    glBufferSubData(GL_ARRAY_BUFFER, vbo->lastByteoffset, vertices->numOfVertices * sizeof(Vertex), vertices->vertexData);//vertices->numOfVertices * sizeof(Vertex)

    //give EBO data
    glGenBuffers(1, &(m->EBO));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices->numOfIndices * sizeof(GLuint),indices->indices,GL_STATIC_DRAW);

    //give mesh data
    m->numOfIndices = indices->numOfIndices;
    glm_vec4_copy(meshPosData-> rotation, m->rotation);
    glm_vec3_copy(meshPosData->translation, m->translation);
    m->VBOVertexOffset = vbo->lastByteoffset/sizeof(Vertex);

    //update vbo
    vbo->append(vbo, m);
    vbo->lastByteoffset += vertices->numOfVertices * sizeof(Vertex);

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
}
*/
/*main*/
void importModel(Model *model, GLuint VAO, VBO* vbo, char *modelPath, GLushort sizeofModelPath) {
    /*creating the process*/
    HANDLE read_pipe, write_pipe;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;

    DWORD bytesRead;
    
    //get stdOut handle
    HANDLE cStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // Create the pipes
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        printf("Failed to create pipe.\n");
        return;
    }

    // Create the process
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdInput = read_pipe;
    si.hStdOutput = write_pipe;
    si.hStdError = cStdOut;//write_pipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    if (!CreateProcess("./venv/Scripts/python.exe"," ./src_py/gltf_importer.py",NULL,&sa,TRUE,0,NULL,NULL,&si,&pi)) {
        printf("Failed to create process.\n");
        return;
    }
    /*make python do the parsing*/
    //const char* modelPath = "\\resourceFiles\\models\\bunny\\scene.gltf\n";
    printf("c writing filePath\n");
    WriteFile(write_pipe, modelPath, sizeofModelPath, NULL, NULL);

    /*recover and put the data where it belongs*/
    //amount of meshes
    unsigned int amountOfMeshes;
    WaitForSingleObject(pi.hProcess,1500);
    ReadFile(read_pipe,&amountOfMeshes,sizeof(unsigned int),&bytesRead,NULL);
    //printf("c recovered %u\n", amountOfMeshes);

    for (int i = 0; i <= amountOfMeshes; i++){
        //vertices
        static vertexBuffer vertices;
        ReadFile(read_pipe, &vertices, sizeof(vertexBuffer), &bytesRead, NULL);
        //printf("c recovered %u bytes\n", bytesRead);
        //printf("vertex: [pos:%f,%f,%f color:%f,%f,%f, imgPos:%f,%f, normal:%f,%f,%f]\n",verticesBuffer[0].pos[0], verticesBuffer[0].pos[1], verticesBuffer[0].pos[2], verticesBuffer[0].color[0],verticesBuffer[0].color[1],verticesBuffer[0].color[2], verticesBuffer[0].imgPos[0], verticesBuffer[0].imgPos[1], verticesBuffer[0].normal[0], verticesBuffer[0].normal[1], verticesBuffer[0].normal[2]);
        //printf("Last vertex: [pos:%f,%f,%f color:%f,%f,%f, imgPos:%f,%f, normal:%f,%f,%f]\n",verticesBuffer[INDEX].pos[0], verticesBuffer[INDEX].pos[1], verticesBuffer[INDEX].pos[2], verticesBuffer[INDEX].color[0],verticesBuffer[INDEX].color[1],verticesBuffer[INDEX].color[2], verticesBuffer[INDEX].imgPos[0], verticesBuffer[INDEX].imgPos[1], verticesBuffer[INDEX].normal[0], verticesBuffer[INDEX].normal[1], verticesBuffer[INDEX].normal[2]);

        //indices
        static indicesBuffer indices;
        ReadFile(read_pipe, &indices, sizeof(indicesBuffer), &bytesRead, NULL);
        //printf("c recovered %u bytes, greenlighting continue\n", bytesRead);

        //pos data
        MeshPosDataBuffer meshPosData;
        ReadFile(read_pipe, &meshPosData, sizeof(MeshPosDataBuffer), &bytesRead, NULL);
        WriteFile(write_pipe,"\n", 1, NULL,NULL);

        Mesh currMesh;
        model->append(model,currMesh);
        createMeshWithBuffers(&(model->Meshes[model->lastIndexAvailable - 1]), &indices,
        &vertices, &meshPosData, VAO, vbo);

    }
    printf("Importing done\n");

    CloseHandle(write_pipe);
    CloseHandle(read_pipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    /*
    printf("\n      model\n");
    printf("EBO: %u\n", model.Meshes[0].EBO);
    printf("numOfIndices: %u\n", model.Meshes[0].numOfIndices);
    printf("rotation: %f,%f,%f,%f\n", model.Meshes[0].rotation[0], model.Meshes[0].rotation[1], model.Meshes[0].rotation[2], model.Meshes[0].rotation[3]);
    printf("translation: %f,%f,%f\n", model.Meshes[0].translation[0], model.Meshes[0].translation[1], model.Meshes[0].translation[2], model.Meshes[0].translation[3]);
    printf("vertex offset: %i\n", model.Meshes[0].VBOVertexOffset);
    */
}
