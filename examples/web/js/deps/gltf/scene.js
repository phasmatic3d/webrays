import {LightManager} from './light_manager.js';
import {createOrbitCamera} from './cameras.js';

export const MATERIAL_TYPE = {
    NO_LIGHTING: 0,
    LAMBERT: 1,
    GGX: 2,
    REFLECTIVE: 3,
    REFRACTIVE: 4
};
Object.freeze(MATERIAL_TYPE);

export const ALPHA_MODE = {
    OPAQUE: 0,
    BLENDED: 1,
    ALPHA_CULL: 2
};
Object.freeze(ALPHA_MODE);

const GGX_MATERIAL = {
    type: MATERIAL_TYPE.GGX,
    baseColor: [1,1,1,1],
    alphaMode: ALPHA_MODE.OPAQUE,
    ior: 1,
    metallic: 0.5,
    roughness: 1.0,
    reflectance: 0.5,
    baseColorTexture : -1,
    metallicRoughnessTexture : -1,
    normalsTexture : -1
};
Object.freeze(GGX_MATERIAL);

function copyMaterial(mat) {
    return {...mat, baseColor: [...mat.baseColor]};
}

function createBBOX()
{
    return {
        minValue: glMatrix.vec3.fromValues(Number.MAX_VALUE, Number.MAX_VALUE, Number.MAX_VALUE),
        maxValue: glMatrix.vec3.fromValues(-Number.MAX_VALUE, -Number.MAX_VALUE, -Number.MAX_VALUE),
    }
}

/// name and options
export function createUnitQuad(name, options)
{
    const defaultOptions = {width: 1, height: 1};
    options = (typeof options) !== "object"? {} : options;
    const {width, height} = {...options, ...defaultOptions};

    const mesh = {
        name: name,
        vertices: new Float32Array([-0.5 * width, -0.5 * height, 0.0, 0.5 * width, -0.5 * height, 0.0, 0.0, 0.5 * height, 0.0]),
        normals: new Float32Array([0, 0, 1, 0, 0, 1, 0, 0, 1]),
        uv: new Float32Array([0, 0, 1, 0, 1, 1, 0, 1])
    };
    
    return mesh;
}

export function createTriangle(name, options)
{
    const defaultOptions = {width: 1, height: 1};
    options = (typeof options) !== "object"? {} : options;
    const {width, height} = {...options, ...defaultOptions};

    const mesh = {
        name: name,
        vertices: new Float32Array([-1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 0.0, 1.0, 0.0]),
        normals: new Float32Array([0, 0, 1]),
        uv: new Float32Array([0, 0, 1, 0, 1, 1, 0, 1])
    };
    
    return mesh;
}

export function createUnitCube(name, options)
{
    const defaultOptions = {width: 1, height: 1, depth: 1, position: [0,0,0]};
    options = (typeof options) !== "object"? {} : options;
    const {width, height, depth, position} = {...defaultOptions, ...options};

    const dataArray = new Float32Array([
        // front
		-0.5, -0.5, 0.5,
		0.5, -0.5, 0.5,
		0.5, 0.5, 0.5,
		-0.5, 0.5, 0.5,
		// back
		-0.5, -0.5, -0.5,
		0.5, -0.5, -0.5,
		0.5, 0.5, -0.5,
		-0.5, 0.5, -0.5]);
    const normalArray = new Float32Array([
        // front
        0, 0, 1, 
        0, 0, 1,
        0, 0, 1,
        0, 0, 1, 
        0, 0, 1,
        0, 0, 1,
        // right
        1, 0, 0, 
        1, 0, 0,
        1, 0, 0,
        1, 0, 0, 
        1, 0, 0,
        1, 0, 0,
        //back
        0, 0, -1, 
        0, 0, -1,
        0, 0, -1,
        0, 0, -1, 
        0, 0, -1,
        0, 0, -1,
        // left
        -1, 0, 0, 
        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0, 
        -1, 0, 0,
        -1, 0, 0,
        // bottom
        0, -1, 0, 
        0, -1, 0,
        0, -1, 0,
        0, -1, 0, 
        0, -1, 0,
        0, -1, 0,
        // top
        0, 1, 0, 
        0, 1, 0,
        0, 1, 0,
        0, 1, 0, 
        0, 1, 0,
        0, 1, 0,
        
    ]);
    const uvArray = new Float32Array([
        0, 0, 1, 0, 1, 1,
        1, 1, 0, 1, 0, 0,
        0, 0, 1, 0, 1, 1,
        1, 1, 0, 1, 0, 0,
        0, 0, 1, 0, 1, 1,
        1, 1, 0, 1, 0, 0,
        0, 0, 1, 0, 1, 1,
        1, 1, 0, 1, 0, 0,
        0, 0, 1, 0, 1, 1,
        1, 1, 0, 1, 0, 0,
        0, 0, 1, 0, 1, 1,
        1, 1, 0, 1, 0, 0]);
    const indicesArray = new Int32Array([
        // front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3        
    ]);

    let dataArray2 = new Float32Array(36 * 3);
    let offset = 0;
    for(let index of indicesArray)
    {
        dataArray2[3 * offset + 0] = width * dataArray[3 * index + 0] + position[0];
        dataArray2[3 * offset + 1] = height * dataArray[3 * index + 1] + position[1];
        dataArray2[3 * offset + 2] = depth * dataArray[3 * index + 2] + position[2];
        offset += 1;
    }
    
    const indicesArray2 = new Int32Array([0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35]);

    const mesh = {
        name: name,
        vertices: dataArray2,
        normals: normalArray,
        uv: uvArray,
        indices: indicesArray2,
        parts: [{offset:0, no_vertices: 36, material: 0}],
        materials: [copyMaterial(GGX_MATERIAL)]
    };
    
    return mesh;
}

export function createMesh(name, options, mesh)
{
    const defaultOptions = {width: 1, height: 1, depth: 1, position: [0,0,0]};
    options = (typeof options) !== "object"? {} : options;
    const {width, height, depth, position} = {...defaultOptions, ...options};

    const r_mesh = {
        name: name,
        matrix: mesh.matrix,
        vertices: mesh.mesh.vertices,
        normals: mesh.mesh.normals,
        uv: mesh.mesh.texcoords,
        indices: mesh.mesh.indices,
        parts: mesh.mesh.parts,
        materials: mesh.materials,
        images: mesh.images
    };
    
    return r_mesh;
}

export function createMeshFromScene(name, options, scene)
{
    const defaultOptions = {width: 1, height: 1, depth: 1, position: [0,0,0]};
    options = (typeof options) !== "object"? {} : options;
    const {width, height, depth, position} = {...defaultOptions, ...options};

    const no_vertices = scene.meshes.reduce((acc, curr) => acc + curr.mesh.vertices.length, 0);
    const no_indices = scene.meshes.reduce((acc, curr) => acc + curr.mesh.indices.length, 0);
    
    const vertices = new Float32Array(no_vertices);
    const normals = new Float32Array(no_vertices);
    const uv = new Float32Array(2 * no_vertices / 3);
    const tangents = new Float32Array(4 * no_vertices / 3);
    const indices = new Uint32Array(no_indices);

    let v_offset = 0;
    let i_offset = 0;
    const parts = [];
    const maxValue = glMatrix.vec3.fromValues(-Number.MAX_VALUE,-Number.MAX_VALUE,-Number.MAX_VALUE);
    const minValue = glMatrix.vec3.fromValues(Number.MAX_VALUE,Number.MAX_VALUE,Number.MAX_VALUE);
    for(const submesh of scene.meshes)
    {
        const matrix = submesh.matrix;
        //const inv_matrix = glMatrix.mat4.transpose(glMatrix.mat4.create(), glMatrix.mat4.invert(glMatrix.mat4.create(), matrix));
        const inv_matrix = glMatrix.mat3.normalFromMat4(glMatrix.mat3.create(), matrix);
        for(let i = 0; i< submesh.mesh.vertices.length / 3; ++i)
        {
            const v = glMatrix.vec3.transformMat4(glMatrix.vec3.create(), new Float32Array(submesh.mesh.vertices.buffer, 4 * 3 * i, 3), matrix);
            vertices[3 * v_offset + 3*i] = v[0];
            vertices[3 * v_offset + 3*i + 1] = v[1];
            vertices[3 * v_offset + 3*i + 2] = v[2];
            glMatrix.vec3.max(maxValue, maxValue, v);
            glMatrix.vec3.min(minValue, minValue, v);

            const n = glMatrix.vec3.transformMat3(glMatrix.vec3.create(), new Float32Array(submesh.mesh.normals.buffer, 4 * 3 * i, 3), inv_matrix);
            glMatrix.vec3.normalize(n,n);
            normals[3 * v_offset + 3 * i] = n[0];
            normals[3 * v_offset + 3 * i + 1] = n[1];
            normals[3 * v_offset + 3 * i + 2] = n[2];

            uv[2 * v_offset + 2 * i] = submesh.mesh.texcoords[2 * i];
            uv[2 * v_offset + 2 * i + 1] = submesh.mesh.texcoords[2 * i + 1];

            if(submesh.mesh.tangents)
            {
                const t = glMatrix.vec3.transformMat3(glMatrix.vec3.create(), new Float32Array(submesh.mesh.tangents.buffer, 4 * 4 * i, 3), inv_matrix);
                glMatrix.vec3.normalize(t,t);
                tangents[4 * v_offset + 4 * i] = t[0];
                tangents[4 * v_offset + 4 * i + 1] = t[1];
                tangents[4 * v_offset + 4 * i + 2] = t[2];
                tangents[4 * v_offset + 4 * i + 3] = submesh.mesh.tangents[4 * i + 3];
            }
        }
        for(let i = 0; i< submesh.mesh.indices.length; ++i)
        {
            indices[i_offset + i] = submesh.mesh.indices[i] + v_offset;
        }
        for(let p of submesh.mesh.parts)
        {
            parts.push({offset: i_offset + p.offset, no_vertices: p.no_vertices, material: p.material});
        }
        v_offset += submesh.mesh.vertices.length / 3;
        i_offset += submesh.mesh.indices.length;        
    }

    const matrix = glMatrix.mat4.create();
    const r_mesh = {
        name: name,
        matrix: matrix,
        vertices: vertices,
        normals: normals,
        uv: uv,
        tangents : tangents,
        indices: indices,
        parts: parts,
        materials: scene.materials,
        images: scene.images,
        dirty: true,
        _position: [0,0,0],
        bbox: {minValue, maxValue},
        set position(pos) {
            this._position[0] = pos[0];
            this._position[1] = pos[1];
            this._position[2] = pos[2];

            this.matrix = glMatrix.mat4.fromTranslation(this.matrix, this._position);
        },
        get position() {
            return this._position;
        }
    };
    
    return r_mesh;
}


export function createScene(engine) {

    const camera = createOrbitCamera(engine.width, engine.height);
    engine.camera = camera; // default camera

    return {
        name: "scene",
        children: [],
        engine: engine,
        camera: camera,
        lights: LightManager(engine),
        bbox: createBBOX(),

        addMesh: function(mesh) {
            this.children.push(mesh);
            this.engine.addMesh(mesh);
        },
        update: function(time) {
            this.camera.update(time);
            this.lights.update(time);
            const invalid = this.children.every(e => e.dirty);
            if(invalid)
            {
                this.bbox = null;
                if(this.children.length === 0)
                    return;
                console.error("Invalid SCENE ", this.children.length);
                // recompute BBOX
                this.bbox = createBBOX();
                for(let i=0; i<this.children.length; ++i)
                {
                    const child = this.children[i];
                    child.dirty = false;
                    glMatrix.vec3.min(this.bbox.minValue, this.bbox.minValue, child.bbox.minValue);
                    glMatrix.vec3.max(this.bbox.maxValue, this.bbox.maxValue, child.bbox.maxValue);
                }
                console.log(this.bbox);
                this.camera.setSceneBBOX(this.bbox);
            }
        }
    }
}