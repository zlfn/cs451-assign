#include "base.hpp"
#include "utils.hpp"
#include "graphics.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

MatrixStack::MatrixStack() { stack.push_back(glm::identity<glm::mat4x4>()); }

void MatrixStack::loadIdentity() { stack.back() = glm::identity<glm::mat4x4>(); }

void MatrixStack::matMul(glm::mat4x4 m) { stack.back() *= m; }

glm::mat4x4 MatrixStack::getTopMatrix() { return stack.back(); }

glm::mat4x4 MatrixStack::getTopNormal() {
    glm::mat4x4 modelView = stack.back();
    glm::mat3x3 modelView_3x3 = glm::mat3x3(modelView);
    glm::mat3x3 inverse_3x3 = glm::inverse(modelView_3x3);
    glm::mat3x3 normalMatrix_3x3 = glm::transpose(inverse_3x3);
    return glm::mat4x4(normalMatrix_3x3);
}

void MatrixStack::matPush() { stack.push_back(stack.back()); }

void MatrixStack::matPop() { stack.pop_back(); }

void MatrixStack::translate(float x, float y, float z) {
    stack.back() = glm::translate(stack.back(), glm::vec3(x, y, z));
}

void MatrixStack::rotate(float angle, float x, float y, float z) {
    // angle은 degree로 받아서 radian으로 변환 (glRotatef와 호환)
    float radians = glm::radians(angle);
    stack.back() = glm::rotate(stack.back(), radians, glm::vec3(x, y, z));
}

void MatrixStack::scale(float x, float y, float z) {
    stack.back() = glm::scale(stack.back(), glm::vec3(x, y, z));
}

ThreeDObj::ThreeDObj(const std::string &FILE_PATH, const std::string &TEXTURE_PATH,
                     const std::string &NORMAL_PATH, const glm::fvec3 &color)
    : objectColor(color) // 기본은 흰색
{
    try {
        getObjFile(FILE_PATH);
        getFragInfo(TEXTURE_PATH, NORMAL_PATH);
    } catch (const std::exception &e) {
        std::cerr << "Error loading object: " << e.what() << '\n';
    }
}

void ThreeDObj::getFragInfo(const std::string &TEXTURE_PATH, const std::string &NORMAL_PATH) {
    std::ifstream tfile(TEXTURE_PATH);
    if (!tfile.is_open()) {
        throw std::runtime_error("Failed to open texture file: " + TEXTURE_PATH);
    }
    
    int tWidth, tHeight, tNrChannels;
    unsigned char *tData = stbi_load(TEXTURE_PATH.c_str(), &tWidth, &tHeight, &tNrChannels, 0);

    if (tData) {
        // 텍스처 객체 생성 및 바인딩
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 텍스처 파라미터 설정 (필터링 및 래핑)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 채널 수에 따른 포맷 결정
        GLenum format = (tNrChannels == 4) ? GL_RGBA : GL_RGB;

        // 텍스처 데이터 GPU로 전송
        glTexImage2D(GL_TEXTURE_2D, 0, format, tWidth, tHeight, 0, format, GL_UNSIGNED_BYTE, tData);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Texture failed to load at path: " << TEXTURE_PATH << std::endl;
    }
    stbi_image_free(tData);

    std::cout << "Successfully loaded Color/Diffuse Texture: " << TEXTURE_PATH << std::endl;

    std::ifstream nfile(NORMAL_PATH);
    if (!nfile.is_open()) {
        throw std::runtime_error("Failed to open normal file: " + NORMAL_PATH);
    }

    int nWidth, nHeight, nNrChannels;
    unsigned char *nData = stbi_load(NORMAL_PATH.c_str(), &nWidth, &nHeight, &nNrChannels, 0);

    if (nData) {
        // 텍스처 객체 생성 및 바인딩
        glGenTextures(1, &normalMapID);
        glBindTexture(GL_TEXTURE_2D, normalMapID);

        // 텍스처 파라미터 설정 (필터링 및 래핑)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 채널 수에 따른 포맷 결정
        GLenum format = (nNrChannels == 4) ? GL_RGBA : GL_RGB;

        // 텍스처 데이터 GPU로 전송
        glTexImage2D(GL_TEXTURE_2D, 0, format, nWidth, nHeight, 0, format, GL_UNSIGNED_BYTE, nData);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Normal Map failed to load at path: " << NORMAL_PATH << std::endl;
    }
    stbi_image_free(nData);

    std::cout << "Successfully loaded Normal Map Texture: " << NORMAL_PATH << std::endl;

    glBindTexture(GL_TEXTURE_2D, 0);
}

void ThreeDObj::getObjFile(const std::string &FILE_PATH) {
    std::ifstream file(FILE_PATH);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open obj file: " + FILE_PATH);
    }

    std::string currentObjName = "base";

    baseVertices.clear();
    baseNormals.clear();
    baseTexCoords.clear();
    objIndicesMap.clear();

    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "o") {
            Indices newIndices;
            ss >> currentObjName;
            objIndicesMap.insert({currentObjName, newIndices});
        } else if (prefix == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            baseVertices.push_back(vertex);
        } else if (prefix == "vn") {
            glm::vec3 vertexNormal;
            ss >> vertexNormal.x >> vertexNormal.y >> vertexNormal.z;
            baseNormals.push_back(vertexNormal);
        } else if (prefix == "vt") {
            glm::vec2 texCoord;
            ss >> texCoord.x >> texCoord.y;
            baseTexCoords.push_back(texCoord);
        } else if (prefix == "f" && currentObjName != "") { // f 처리
            std::vector<IndexInfo> faceIndices;
            std::string vertexToken;

            while (ss >> vertexToken) {
                std::stringstream tokenSS(vertexToken);
                std::string indexStr;
                IndexInfo indices;

                // v
                std::getline(tokenSS, indexStr, '/');
                indices.v_index = std::stoul(indexStr) - 1;

                // vt
                std::getline(tokenSS, indexStr, '/');
                if (!indexStr.empty()) {
                    indices.vt_index = std::stoul(indexStr) - 1;
                } else {
                    indices.vt_index = 0;
                }

                std::getline(tokenSS, indexStr);
                if (!indexStr.empty()) {
                    indices.vn_index = std::stoul(indexStr) - 1;
                } else {
                    indices.vn_index = 0;
                }

                faceIndices.push_back(indices);
            }

            for (size_t i = 1; i < faceIndices.size() - 1; ++i) {
                objIndicesMap[currentObjName].push_back(faceIndices[0]);
                objIndicesMap[currentObjName].push_back(faceIndices[i]);
                objIndicesMap[currentObjName].push_back(faceIndices[i + 1]);
            }
        }
    }
    file.close();

    for (const auto &[key, value] : objIndicesMap) {
        Indices currentIndices = value;
        glm::vec3 centerPos = glm::vec3(0.0, 0.0, 0.0);
        for (const auto &vIndex : currentIndices) {
            centerPos += baseVertices[vIndex.v_index];
        }
        centerPos /= currentIndices.size();
        objCenterMap.insert({key, centerPos});
    }

    std::cout << "Loaded " << baseVertices.size() << " vertices, " << baseTexCoords.size()
              << " texture coordinates, "
              << (objIndicesMap.size()) << " objects from " << FILE_PATH << std::endl;
}

void ThreeDObj::setColor(const glm::vec3 &color) {
    objectColor = color;
    objMeshMap.clear(); // color 변경 시 모든 Mesh 재생성 필요
}

void ThreeDObj::createMesh(const std::string &objName) {
    Indices indices;
    try {
        indices = objIndicesMap.at(objName);
    } catch (const std::out_of_range &) {
        return;
    }

    std::vector<float> vertexData;
    vertexData.reserve(indices.size() * 8); // 3 pos + 3 norm + 2 tex

    for (IndexInfo index : indices) {
        const glm::vec3 &vertex = baseVertices[index.v_index];
        const glm::vec3 &vertexNormal = baseNormals[index.vn_index];
        
        glm::vec2 texCoord(0.0f, 0.0f);
        if (!baseTexCoords.empty() && index.vt_index < baseTexCoords.size()) {
             texCoord = baseTexCoords[index.vt_index];
        }

        vertexData.push_back(vertex.x);
        vertexData.push_back(vertex.y);
        vertexData.push_back(vertex.z);
        vertexData.push_back(vertexNormal.x);
        vertexData.push_back(vertexNormal.y);
        vertexData.push_back(vertexNormal.z);
        vertexData.push_back(texCoord.x);
        vertexData.push_back(texCoord.y);
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->setData(vertexData.data(), vertexData.size() * sizeof(float), GL_STATIC_DRAW);
    mesh->setAttribute(0, 3, GL_FLOAT, 8 * sizeof(float), (void *)0);
    mesh->setAttribute(1, 3, GL_FLOAT, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    mesh->setAttribute(3, 2, GL_FLOAT, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    mesh->setDrawMode(GL_TRIANGLES, (GLsizei)indices.size());

    objMeshMap[objName] = std::move(mesh);
}

void ThreeDObj::draw(const std::string objName) {
    if (!g_shaderProgram || baseVertices.empty() || objIndicesMap.empty()) {
        return;
    }

    // Mesh가 없으면 생성
    if (objMeshMap.find(objName) == objMeshMap.end()) {
        createMesh(objName);
    }
    if (objMeshMap.find(objName) == objMeshMap.end()) {
        std::cerr << "Error: Object '" << objName << "' not found\n";
        return;
    }

    // Translation 적용
    glm::vec3 translation(0.0f);
    if (objTranslationMap.find(objName) != objTranslationMap.end()) {
        translation = objTranslationMap.at(objName);
    }

    modelViewStack.matPush();
    modelViewStack.translate(translation.x, translation.y, translation.z);

    glm::mat4 projection = projectionStack.getTopMatrix();
    glm::mat4 modelView = modelViewStack.getTopMatrix();
    glm::mat4 normalMat = modelViewStack.getTopNormal();
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMapID);

    drawMesh(*objMeshMap[objName], *g_shaderProgram, [&](const ShaderProgram& prog) {
        prog.setUniform("projection", projection);
        prog.setUniform("modelView", modelView);
        prog.setUniform("normalMatrix", normalMat);
        prog.setUniform("objectColor", objectColor);
        prog.setUniform("useVertexColor", 1.0f); // 1.0 to multiply texture by lighting (Gouraud) or pass-through
        prog.setUniform("colorSampler", 0);
        prog.setUniform("normalSampler", 1);
        prog.setUniform("useTexture", 1.0f); 
    });

    modelViewStack.matPop();
}

void ThreeDObj::drawAll() {
    if (baseVertices.empty() || objIndicesMap.empty()) {
        return;
    }

    // 하위 객체 순회
    for (const auto &pair : objIndicesMap) {
        const std::string &objName = pair.first;
        draw(objName);
    }
}

// Helper: 평면과 두 정점 사이의 교차점 계산
glm::vec3 ThreeDObj::intersectPlane(const glm::vec3 &p1, const glm::vec3 &p2, float z_plane) {
    float t = (z_plane - p1.z) / (p2.z - p1.z);
    return glm::mix(p1, p2, t);
}

// 헬퍼: 객체 중심 계산
glm::vec3 ThreeDObj::calculateCenter(const Indices &indices) {
    glm::vec3 centerPos(0.0f);
    if (indices.empty())
        return centerPos;

    // 인덱스 다 더하기
    for (const auto &index : indices) {
        if (index.v_index < baseVertices.size()) {
            centerPos += baseVertices[index.v_index];
        }
    }
    return centerPos / (float)indices.size();
}

// helper: 연결 요소 분리
void ThreeDObj::findConnectedComponents(Indices &all_indices, const std::string &objName,
                                        const std::string &suffix) {
    if (all_indices.empty())
        return;

    // (v1, v2) pair -> 삼각형 인덱스 리스트 만들기
    using Edge = std::pair<unsigned int, unsigned int>;
    auto make_edge = [](unsigned int v1, unsigned int v2) {
        return (v1 < v2) ? std::make_pair(v1, v2) : std::make_pair(v2, v1);
    };

    size_t num_tris = all_indices.size() / 3;
    std::map<Edge, std::vector<size_t>> edge_to_tri_map;

    // 삼각형 인덱스 -> 인접한 삼각형 인덱스 set 만들기
    std::map<size_t, std::set<size_t>> tri_adj_graph;

    // 모서리-삼각형 맵
    for (size_t i = 0; i < num_tris; ++i) {
        unsigned int v0 = all_indices[i * 3].v_index;
        unsigned int v1 = all_indices[i * 3 + 1].v_index;
        unsigned int v2 = all_indices[i * 3 + 2].v_index;
        edge_to_tri_map[make_edge(v0, v1)].push_back(i);
        edge_to_tri_map[make_edge(v1, v2)].push_back(i);
        edge_to_tri_map[make_edge(v2, v0)].push_back(i);
    }

    // 삼각형-삼각형 인접 그래프
    for (const auto &pair : edge_to_tri_map) {
        const auto &tri_list = pair.second;
        if (tri_list.size() == 2) { // 하나의 모서리를 두 삼각형이 공유한다면 (내부 모서리)
            tri_adj_graph[tri_list[0]].insert(tri_list[1]);
            tri_adj_graph[tri_list[1]].insert(tri_list[0]);
        }
    }

    // BFS로 연결 요소 찾기
    std::set<size_t> unvisited_tris;
    for (size_t i = 0; i < num_tris; ++i)
        unvisited_tris.insert(i);

    int component_count = 0;
    while (!unvisited_tris.empty()) {
        component_count++;
        std::string new_obj_name = objName + suffix + "_" + std::to_string(component_count);
        Indices new_component_indices;

        std::queue<size_t> q;
        size_t start_tri = *unvisited_tris.begin(); // 방문하지 않은 첫 삼각형에서 시작
        q.push(start_tri);
        unvisited_tris.erase(start_tri);

        while (!q.empty()) { // BFS 루프
            size_t curr_tri = q.front();
            q.pop();

            // 삼각형을 새 컴포넌트에 추가
            new_component_indices.push_back(all_indices[curr_tri * 3]);
            new_component_indices.push_back(all_indices[curr_tri * 3 + 1]);
            new_component_indices.push_back(all_indices[curr_tri * 3 + 2]);

            // 인접한 삼각형들을 큐에 추가
            if (tri_adj_graph.count(curr_tri)) {
                for (size_t adj_tri : tri_adj_graph.at(curr_tri)) {
                    if (unvisited_tris.count(adj_tri)) { // 아직 방문 안 했다면
                        unvisited_tris.erase(adj_tri);
                        q.push(adj_tri);
                    }
                }
            }
        }

        // 찾은 컴포넌트를 맵에 추가
        objIndicesMap[new_obj_name] = new_component_indices;
        objCenterMap[new_obj_name] = calculateCenter(new_component_indices);
    }
}

// 객체를 z=0 기준 분할
void ThreeDObj::splitObjectByZPlane(const std::string &objName, float z_plane) {
    if (objIndicesMap.find(objName) == objIndicesMap.end()) {
        std::cerr << "Error: Object '" << objName << "' not found." << std::endl;
        return;
    }

    Indices original_indices = objIndicesMap[objName]; // 원본 값 복사
    Indices all_above_indices;
    Indices all_below_indices;

    // 정점 인덱스 pair -> 새로운 교차점 인덱스 만들기
    std::map<std::pair<IndexInfo, IndexInfo>, IndexInfo> intersection_cache;

    // 교차점 생성/조회
    auto get_or_create_intersection = [&](IndexInfo idx1, IndexInfo idx2) -> IndexInfo {
        std::pair<IndexInfo, IndexInfo> key = (idx1.v_index < idx2.v_index) ? std::make_pair(idx1, idx2) : std::make_pair(idx2, idx1);
        if (intersection_cache.find(key) != intersection_cache.end()) {
            return intersection_cache[key];
        }

        glm::vec3 v1 = baseVertices[idx1.v_index];
        glm::vec3 v2 = baseVertices[idx2.v_index];
        const glm::vec3 &n1 = baseNormals[idx1.vn_index];
        const glm::vec3 &n2 = baseNormals[idx2.vn_index];

        glm::vec3 intersection_vert = intersectPlane(v1, v2, z_plane);
        float t = (z_plane - v1.z) / (v2.z - v1.z);
        glm::vec3 intersection_normal = glm::normalize(glm::mix(n1, n2, t));

        baseVertices.push_back(intersection_vert);
        size_t new_v_index = baseVertices.size() - 1;

        baseNormals.push_back(intersection_normal);
        size_t new_vn_index = baseNormals.size() - 1;
        IndexInfo new_index = IndexInfo(new_v_index, new_vn_index);

        intersection_cache[key] = new_index;
        return new_index;
    };

    // 클리핑
    for (size_t i = 0; i < original_indices.size(); i += 3) {
        IndexInfo ii0 = original_indices[i];
        IndexInfo ii1 = original_indices[i + 1];
        IndexInfo ii2 = original_indices[i + 2];
        
        unsigned int i0 = ii0.v_index;
        unsigned int i1 = ii1.v_index;
        unsigned int i2 = ii2.v_index;

        glm::vec3 v0 = baseVertices[i0];
        glm::vec3 v1 = baseVertices[i1];
        glm::vec3 v2 = baseVertices[i2];

        const float EPSILON = 1e-6f;
        float z0 = v0.z - z_plane;
        float z1 = v1.z - z_plane;
        float z2 = v2.z - z_plane;

        int above_count = (z0 > EPSILON) + (z1 > EPSILON) + (z2 > EPSILON);
        int below_count = (z0 < -EPSILON) + (z1 < -EPSILON) + (z2 < -EPSILON);

        // 모두 위인 trivial 케이스
        if (below_count == 0) {
            all_above_indices.push_back(ii0);
            all_above_indices.push_back(ii1);
            all_above_indices.push_back(ii2);
        }
        // 모두 아래인 trivial 케이스
        else if (above_count == 0) {
            all_below_indices.push_back(ii0);
            all_below_indices.push_back(ii1);
            all_below_indices.push_back(ii2);
        }
        // 클리핑이 필요한 케이스
        else {
            if (above_count == 1 || below_count == 1) {
                bool single_is_above = (above_count == 1);
                IndexInfo i_solo, i_p1, i_p2;

                if (single_is_above) {
                    if (z0 > EPSILON) {
                        i_solo = ii0;
                        i_p1 = ii1;
                        i_p2 = ii2;
                    } else if (z1 > EPSILON) {
                        i_solo = ii1;
                        i_p1 = ii2;
                        i_p2 = ii0;
                    } else {
                        i_solo = ii2;
                        i_p1 = ii0;
                        i_p2 = ii1;
                    }
                } else {
                    if (z0 < -EPSILON) {
                        i_solo = ii0;
                        i_p1 = ii1;
                        i_p2 = ii2;
                    } else if (z1 < -EPSILON) {
                        i_solo = ii1;
                        i_p1 = ii2;
                        i_p2 = ii0;
                    } else {
                        i_solo = ii2;
                        i_p1 = ii0;
                        i_p2 = ii1;
                    }
                }

                IndexInfo i_int1 = get_or_create_intersection(i_solo, i_p1);
                IndexInfo i_int2 = get_or_create_intersection(i_solo, i_p2);

                if (single_is_above) {
                    // 위쪽이 작은 삼각형
                    all_above_indices.push_back(i_solo);
                    all_above_indices.push_back(i_int1);
                    all_above_indices.push_back(i_int2);
                    // 아래쪽이 사각형 -> 삼각형 2개로 쪼개기
                    all_below_indices.push_back(i_p1);
                    all_below_indices.push_back(i_p2);
                    all_below_indices.push_back(i_int2);
                    all_below_indices.push_back(i_p1);
                    all_below_indices.push_back(i_int2);
                    all_below_indices.push_back(i_int1);
                } else {
                    // 아래쪽이 작은 삼각형
                    all_below_indices.push_back(i_solo);
                    all_below_indices.push_back(i_int1);
                    all_below_indices.push_back(i_int2);
                    // 위쪽이 사각형 -> 삼각형 2개로 쪼개기
                    all_above_indices.push_back(i_p1);
                    all_above_indices.push_back(i_p2);
                    all_above_indices.push_back(i_int2);
                    all_above_indices.push_back(i_p1);
                    all_above_indices.push_back(i_int2);
                    all_above_indices.push_back(i_int1);
                }
            }
        }
    }

    objIndicesMap.erase(objName);
    objCenterMap.erase(objName);

    // 연결 요소 분리
    findConnectedComponents(all_above_indices, objName, "_above");
    findConnectedComponents(all_below_indices, objName, "_below");

    std::cout << "Object '" << objName << "' split into new components." << std::endl;
    std::cout << "New total vertices: " << baseVertices.size() << std::endl;
}

// 하위 객체를 중심으로부터 일정 거리만큼 분리
void ThreeDObj::separate(float separation_step) {
    if (objCenterMap.size() < 2) { // 개수가 1개 이하면 의미 x
        return;
    }

    // 모든 하위 객체 중심의 평균(그냥 물체들 원점임) 계산
    glm::vec3 globalCenter(0.0f);
    for (const auto &pair : objCenterMap) {
        globalCenter += pair.second;
    }
    globalCenter /= (float)objCenterMap.size();

    // 객체 이동 방향 & 거리 계산
    std::map<std::string, glm::vec3> step_translations;
    for (const auto &pair : objCenterMap) {
        const std::string &objName = pair.first;
        const glm::vec3 &subobjectCenter = pair.second;

        // '폭발 원점'에서 '객체 중심'으로의 방향 벡터
        glm::vec3 direction = subobjectCenter - globalCenter; // 방향 벡터

        if (glm::length(direction) > 1e-6f) { // 거리가 0이 아니면
            direction = glm::normalize(direction);
        } else { // 거리 0이면 그냥 위로 보내기
            direction = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        step_translations[objName] = direction * separation_step; // 이동하는 정도
    }

    // 계산한 이동량 적용하기
    for (auto &pair : objCenterMap) {
        const std::string &objName = pair.first;
        const glm::vec3 &translation_this_step = step_translations[objName];

        if (objTranslationMap.find(objName) == objTranslationMap.end()) {
            objTranslationMap[objName] = translation_this_step;
        } else {
            objTranslationMap[objName] += translation_this_step;
        }

        pair.second += translation_this_step;
    }
}

void showVictoryScreen(const GameState &gameState) {
    int elapsedTime = glutGet(GLUT_ELAPSED_TIME);
    int seconds = elapsedTime / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;

    std::cout << "\n\n";
    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[1;33m"
              << "                                                                             \n";
    std::cout << "\033[1;33m"
              << "       ██╗   ██╗██╗ ██████╗████████╗ ██████╗ ██████╗ ██╗   ██╗██╗            \n";
    std::cout << "\033[1;33m"
              << "       ██║   ██║██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚██╗ ██╔╝██║            \n";
    std::cout << "\033[1;33m"
              << "       ██║   ██║██║██║        ██║   ██║   ██║██████╔╝ ╚████╔╝ ██║            \n";
    std::cout << "\033[1;33m"
              << "       ╚██╗ ██╔╝██║██║        ██║   ██║   ██║██╔══██╗  ╚██╔╝  ╚═╝            \n";
    std::cout << "\033[1;33m"
              << "        ╚████╔╝ ██║╚██████╗   ██║   ╚██████╔╝██║  ██║   ██║   ██╗            \n";
    std::cout << "\033[1;33m"
              << "         ╚═══╝  ╚═╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝            \n";
    std::cout << "\033[1;33m"
              << "                                                                             \n";
    if (gameState.konamiUsed) {
        std::cout
            << "\033[1;35m"
            << "                             ↑↑↓↓←→←→BA                                    \n";
    } else {
        std::cout << "\033[1;35m"
                  << "                          ⟡ BOSS DEFEATED ⟡                              \n";
    }
    std::cout << "\033[1;36m"
              << "============================================================================\n\n";

    std::cout << "\033[1;32m" << "                        ╔════════════════════╗\n";
    std::cout << "\033[1;32m" << "                        ║   GAME STATISTICS  ║\n";
    std::cout << "\033[1;32m" << "                        ╚════════════════════╝\n\n";

    std::cout << "\033[1;37m" << "                    ⏱  Clear Time: " << "\033[1;33m";
    std::cout << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0')
              << std::setw(2) << seconds << "\033[0m\n\n";

    std::cout << "\033[1;37m" << "                    ❤  Lives Remaining: " << "\033[1;31m";
    for (int i = 0; i < gameState.playerHealth; i++) {
        std::cout << "♥";
    }
    std::cout << " (" << gameState.playerHealth << "/" << gameState.MAX_PLAYER_HEALTH
              << ")\033[0m\n\n";

    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[1;35m"
              << "                      Thank you for playing!                              \n";
    std::cout << "\033[1;36m"
              << "============================================================================\n";
    std::cout << "\033[0m\n\n";
}

// a, b를 포함하는 범위에서 랜덤 정수 반환
int getRandomRange(int a, int b) {
    static std::uniform_int_distribution<int> distInt(a, b);
    return static_cast<int>(dist(gen));
}

// 셰이더 기반 사각형 그리기 (Glow 효과는 생략)
void drawRectWithGlow(float x, float y, float width, float height, glm::fvec4 color, float glowSize,
                      float zDepth) {
    if (!g_shaderProgram) return;

    float halfW = width / 2.0f;
    float halfH = height / 2.0f;

    // 정점 데이터 (2 triangles = 6 vertices)
    float vertices[] = {
        // Triangle 1
        x - halfW, y - halfH, zDepth,  color.r, color.g, color.b,
        x + halfW, y - halfH, zDepth,  color.r, color.g, color.b,
        x + halfW, y + halfH, zDepth,  color.r, color.g, color.b,

        // Triangle 2
        x + halfW, y + halfH, zDepth,  color.r, color.g, color.b,
        x - halfW, y + halfH, zDepth,  color.r, color.g, color.b,
        x - halfW, y - halfH, zDepth,  color.r, color.g, color.b
    };

    Mesh mesh;
    mesh.setData(vertices, sizeof(vertices), GL_DYNAMIC_DRAW);
    mesh.setAttribute(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    mesh.setAttribute(1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    mesh.setDrawMode(GL_TRIANGLES, 6);

    glm::mat4 projection = projectionStack.getTopMatrix();
    glm::mat4 modelView = modelViewStack.getTopMatrix();

    drawMesh(mesh, *g_shaderProgram, [&](const ShaderProgram& prog) {
        prog.setUniform("projection", projection);
        prog.setUniform("modelView", modelView);
        prog.setUniform("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // Use white so vertex color isn't tinted
        prog.setUniform("useVertexColor", 1.0f);
    });
}
