import {MATERIAL_TYPE, ALPHA_MODE} from './scene.js';
export {MATERIAL_TYPE, ALPHA_MODE};

function computeSmoothNormals(vertices, indices)
{
    // TODO
    const normals = new Float32Array(vertices.length);
    for(let i=1; i<vertices.length; i+=3)
        normals[i] = 1.0;

    return normals;

    for(let ind = 0; ind < indices.length; ind += 3)
    {
        const i0 = indices[3*ind + 0];
        const v0 = vertices.subarray(i0)
        const i1 = indices[3*ind + 1];
        const v1 = vertices.subarray(i1)
        const i2 = indices[3*ind + 2];
        const v2 = vertices.subarray(i2)
        const a = glMatrix.vec3.sub(glMatrix.vec3.create(), v1, v0);
        glMatrix.vec3.normalize(a, a);
        const b = glMatrix.vec3.sub(glMatrix.vec3.create(), v2, v0);
        glMatrix.vec3.normalize(b, b);
        const n = glMatrix.vec3.cross(out, a, b);
        normals[i0 + 0] += n[0];
        normals[i1 + 0] += n[0];
        normals[i2 + 0] += n[0];

        normals[i0 + 1] += n[1];
        normals[i1 + 1] += n[1];
        normals[i2 + 1] += n[1];

        normals[i0 + 2] += n[2];
        normals[i1 + 2] += n[2];
        normals[i2 + 2] += n[2];

    }
    for(let i=0; i<vertices.length; i+=3)
    {
        const n = normals.subarray(i);
        glMatrix.vec3.normalize(n, n);
    }
    
    return normals;
}

function computeTangents(vertices, normals, texcoords, indices)
{
    const num_vertices = vertices.length / 3;
    const vec3 = glMatrix.vec3;
    const vec2 = glMatrix.vec2;
    const tangents = new Float32Array(4 * num_vertices);
    const bitangents = new Float32Array(4 * num_vertices);
    for (let i = 0; i < indices.length; i += 3)
	{
        const i0 = indices[i];
        const i1 = indices[i + 1];
        const i2 = indices[i + 2];

        const v0 = vertices.subarray(3 * i0, 3 * i0 + 3);
        const v1 = vertices.subarray(3 * i1, 3 * i1 + 3);
        const v2 = vertices.subarray(3 * i2, 3 * i2 + 3);
		        
        const uv0 = texcoords.subarray(2 * i0, 2 * i0 + 2);
        const uv1 = texcoords.subarray(2 * i1, 2 * i1 + 2);
        const uv2 = texcoords.subarray(2 * i2, 2 * i2 + 2);

		// edges of the triangle : position delta
		const deltaPos1 = vec3.subtract(vec3.create(), v1, v0);
        const deltaPos2 = vec3.subtract(vec3.create(), v2, v0);
		
		// uv delta
        const deltaUV1 = vec2.subtract(vec2.create(), uv1, uv0);
        const deltaUV2 = vec2.subtract(vec2.create(), uv2, uv0);
		
		const r = 1.0 / (deltaUV1[0] * deltaUV2[1] - deltaUV1[1] * deltaUV2[0]);
        tangents[4 * i0 + 0] = (deltaPos1[0] * deltaUV2[1] - deltaPos2[0] * deltaUV1[1]) * r;
        tangents[4 * i0 + 1] = (deltaPos1[1] * deltaUV2[1] - deltaPos2[1] * deltaUV1[1]) * r;
        tangents[4 * i0 + 2] = (deltaPos1[2] * deltaUV2[1] - deltaPos2[2] * deltaUV1[1]) * r;
        tangents[4 * i0 + 3] = 1;
        tangents[4 * i1 + 0] = (deltaPos1[0] * deltaUV2[1] - deltaPos2[0] * deltaUV1[1]) * r;
        tangents[4 * i1 + 1] = (deltaPos1[1] * deltaUV2[1] - deltaPos2[1] * deltaUV1[1]) * r;
        tangents[4 * i1 + 2] = (deltaPos1[2] * deltaUV2[1] - deltaPos2[2] * deltaUV1[1]) * r;
        tangents[4 * i1 + 3] = 1;
        tangents[4 * i2 + 0] = (deltaPos1[0] * deltaUV2[1] - deltaPos2[0] * deltaUV1[1]) * r;
        tangents[4 * i2 + 1] = (deltaPos1[1] * deltaUV2[1] - deltaPos2[1] * deltaUV1[1]) * r;
        tangents[4 * i2 + 2] = (deltaPos1[2] * deltaUV2[1] - deltaPos2[2] * deltaUV1[1]) * r;
        tangents[4 * i2 + 3] = 1;
		//glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y)*r;
		//glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x)*r;

        bitangents[4 * i0 + 0] = (deltaPos2[0] * deltaUV2[0] - deltaPos1[0] * deltaUV1[0]) * r;
        bitangents[4 * i0 + 1] = (deltaPos2[1] * deltaUV2[0] - deltaPos1[1] * deltaUV1[0]) * r;
        bitangents[4 * i0 + 2] = (deltaPos2[2] * deltaUV2[0] - deltaPos1[2] * deltaUV1[0]) * r;
        bitangents[4 * i0 + 3] = 1;
        bitangents[4 * i1 + 0] = (deltaPos2[0] * deltaUV2[0] - deltaPos1[0] * deltaUV1[0]) * r;
        bitangents[4 * i1 + 1] = (deltaPos2[1] * deltaUV2[0] - deltaPos1[1] * deltaUV1[0]) * r;
        bitangents[4 * i1 + 2] = (deltaPos2[2] * deltaUV2[0] - deltaPos1[2] * deltaUV1[0]) * r;
        bitangents[4 * i1 + 3] = 1;
        bitangents[4 * i2 + 0] = (deltaPos2[0] * deltaUV2[0] - deltaPos1[0] * deltaUV1[0]) * r;
        bitangents[4 * i2 + 1] = (deltaPos2[1] * deltaUV2[0] - deltaPos1[1] * deltaUV1[0]) * r;
        bitangents[4 * i2 + 2] = (deltaPos2[2] * deltaUV2[0] - deltaPos1[2] * deltaUV1[0]) * r;
        bitangents[4 * i2 + 3] = 1;

		/*{
			glm::vec3 t = (deltaUV2.y * deltaPos1 - deltaUV1.y * deltaPos2);
			t = glm::normalize(t);
		}*/
	}

    for (let i = 0; i < num_vertices; i++)
    {
        const t = tangents.subarray(4 * i, 4 * i + 3);
        const n = normals.subarray(3 * i, 3 * i + 3);
        const b = bitangents.subarray(4 * i, 4 * i + 3);

        // Gram-Schmidt orthogonalize
        const dot = vec3.dot(n, t);
        t[0] = t[0] - n[0] * dot;
        t[1] = t[1] - n[1] * dot;
        t[2] = t[2] - n[2] * dot;
        vec3.normalize(t, t);

        // Calculate handedness
		/*if (vec3.dot(vec3.cross(vec3.create(), n, t), b) < 0.0){
			tangents[4 * i + 3 ] = -1.0; //  t = t * -1.0;
		}*/
    }

    /*for (unsigned int i = 0; i<mesh->vertices.size(); i += 1)
	{
		glm::vec3 & n = mesh->normals[i];
		glm::vec3 & t = mesh->tangents[i];
		glm::vec3 & b = mesh->bitangents[i];

		// Gram-Schmidt orthogonalize
		t = glm::normalize(t - n * glm::dot(n, t));

		// Calculate handedness
		if (glm::dot(glm::cross(n, t), b) < 0.0f){
			t = t * -1.0f;
		}

	}*/
    return tangents;
}

function decodeTexcoords(texcoords )
{
    if(typeof texcoords === 'Float32Array')
        return texcoords;
    else if(typeof texcoords === 'Uint8Array')
        return new Float32Array(texcoords.length).map((e,i) => { return texcoords[i] / 255.0; });
    else if(typeof texcoords === 'Uint16Array')
        return new Float32Array(texcoords.length).map((e,i) => { return texcoords[i] / 65536.0; });
    return texcoords;
}

// get the 3 components of the supplied 4
function decodeTangents(tangents, num_vertices)
{
    let arr = new Float32Array(3 * num_vertices);
    if(tangents === null)
        return arr;
    for(let i = 0; i < num_vertices; ++i)
    {
        //const scale = tangents[4 * i + 3];
        const scale = 1.0;
        arr[3 * i + 0] = scale * tangents[4 * i];
        arr[3 * i + 1] = scale * tangents[4 * i + 1];
        arr[3 * i + 2] = scale * tangents[4 * i + 2];
    }
    return arr;
}
function decodeTangents4(tangents, num_vertices)
{
    if(tangents === null)
        return new Float32Array(4 * num_vertices);
    else
        return tangents;
}

function getTypedArray(type, array, byteOffset, length) 
{
    if(type == 5120) // BYTE
        return new Int8Array(array, byteOffset, length);
    else if(type == 5121) // UNSIGNED BYTE
        return new Uint8Array(array, byteOffset, length);
    else if(type == 5122) // SHORT
        return new Int16Array(array, byteOffset, length);
    else if(type == 5123) // UNSIGNED SHORT
        return new Uint16Array(array, byteOffset, length);
    else if(type == 5125) // UNSIGNED INT
        return new Uint32Array(array, byteOffset, length);
    else if(type == 5126) // FLOAT
        return new Float32Array(array, byteOffset, length);
};

function newTypedArray(type, length) 
{
    if(type == 5120) // BYTE
        return new Int8Array(length);
    else if(type == 5121) // UNSIGNED BYTE
        return new Uint8Array(length);
    else if(type == 5122) // SHORT
        return new Int16Array(length);
    else if(type == 5123) // UNSIGNED SHORT
        return new Uint16Array(length);
    else if(type == 5125) // UNSIGNED INT
        return new Uint32Array(length);
    else if(type == 5126) // FLOAT
        return new Float32Array(length);
};

function toAlphaMode(mode)
{
    if(mode === "BLEND")
        return ALPHA_MODE.BLENDED;
    else if(mode === "MASK")
        return ALPHA_MODE.ALPHA_CULL;
    else // "OPAQUE"
        return ALPHA_MODE.OPAQUE;
}

function getTextureExtensionOffsetScale(extensions)
{
    if(extensions)
    {
        const texture_transform = extensions.KHR_texture_transform;
        if(texture_transform)
        {
            const offset = texture_transform.offset? texture_transform.offset : [0,0];
            const scale = texture_transform.scale? texture_transform.scale : [1,1];
            //console.log("Offset/Scale ", offset, scale);
            return {offset: offset, scale: scale};
        }
    }
    return {offset: [0,0], scale: [1,1]};
}


const JSON_TYPE = 0x4E4F534A;
const BUFFER_TYPE = 0x004E4942;

export async function load_glb2(url)
{
    const glb = await fetch(url).then(res => res.arrayBuffer()).catch(e => {
        console.error(e);
        return null;
    });
    if(glb === null)
        return null;
    const input32 = new Uint32Array(glb);
    const input8 = new Uint8Array(glb);

    const magic = input32[0];
    if(magic !== 0x46546C67)
    {
        console.log("GLB incorrect Magic", magic);
        return null;
    }
    const version = input32[1];
    if(version !== 2)
    {
        console.log("GLB incorrect Version", version);
        return null;
    }
    const length = input32[2];
    if(length !== glb.byteLength)
        console.log("GLB incorrect size", length, "vs", glb.byteLength);
    
    let chunkLength = input32[3];
    let chunkType = input32[4];
    let byteOffset = 5 * 4;
    let chunkData = input8.subarray(byteOffset, byteOffset + chunkLength);

    if(chunkType !== JSON_TYPE)
        console.log("GLB should start with JSON data");

    let utf8decoder = new TextDecoder()
    const gltf = JSON.parse(utf8decoder.decode(chunkData));

    console.log("GLB - GLTF JSON", gltf);
    //console.log(JSON.stringify(gltf, null, '\t'));

    byteOffset += chunkLength;
    const buffers = [];
    for(let buffer of gltf.buffers)
    {
        chunkLength = input32[byteOffset/4 + 0];
        chunkType = input32[byteOffset/4 + 1];
        byteOffset += 2 * 4; // (chunkLength + chunkType)
        if(chunkType === BUFFER_TYPE)
        {
            //console.log("Buffer Length ", chunkLength);
            //chunkData = input8.subarray(byteOffset, byteOffset + chunkLength);
            chunkData = input8.slice(byteOffset, byteOffset + chunkLength).buffer;
            buffer.data = chunkData;
            buffers.push(chunkData);
        }
        else
        {
            console.log("Expecting Binary Chunk data", chunkType);       
            buffers.push(new ArrayBuffer());
        }
        
        byteOffset += chunkLength;
    }
    console.log("GLB - GLTF", gltf);

    const images = gltf.images? await Promise.all(gltf.images.map(async img => {
        if(typeof img.bufferView !== 'undefined')
        {
            // get from a bufferview
            //console.log(img);
            //console.log(gltf.bufferViews[img.bufferView]);
            const bv = gltf.bufferViews[img.bufferView];
            const bv_offset = bv.byteOffset? bv.byteOffset : 0;
            //const data = buffers[bv.buffer].subarray(bv_offset, bv_offset + bv.byteLength);
            const data = buffers[bv.buffer].slice(bv_offset, bv_offset + bv.byteLength); // new buffer
            //console.log(data, img.mimeType);
            const blob = new Blob([data], {type: img.mimeType})
            //console.log(blob);
            //return createImageBitmap(blob);
            const image = await createImageBitmap(blob);
            //console.log(image);
            return createImageBitmap(image, {resizeWidth: 1024, resizeHeight: 1024})
        }
        else
        {
            let res = await (img.uri.startsWith("data:")? fetch(img.uri) : fetch(path+img.uri));
            const blob = await res.blob();
            //console.log(blob);
            return createImageBitmap(blob);
        }
    })) : [];

    console.log(images);
    console.log(gltf);
    //return gltf;
    return await parse_gltf2(gltf, buffers, images);
}

export async function load_gltf2(url)
{
    const path_temp = url.split('/');
    path_temp.pop();
    const path = path_temp.join('/') + "/";

    const gltf = await fetch(url).then(res => res.json()).catch(e => {
        console.error(e);
        return null;
    });
    if(gltf === null)
        return null;

    console.log("GLTF", gltf);
    
    if(gltf.asset.version !== "2.0")
        throw "Incorrect version";

    // Fetch All buffers
    const buffers = await Promise.all(gltf.buffers.map(async b => {
        
        let res = await (b.uri.startsWith("data:")? fetch(b.uri) : fetch(path+b.uri));
        return res.arrayBuffer();
    })); // TODO check fetched size of buffers

    const images = gltf.images? await Promise.all(gltf.images.map(async img => {
        if(typeof img.bufferView !== 'undefined')
        {
            // get from a bufferview
            const bv = gltf.bufferViews[img.bufferView];
            const bv_offset = bv.byteOffset? bv.byteOffset : 0;
            //const data = new Int8Array(buffers[bv.buffer], bv_offset, bv.byteLength);
            const data = buffers[bv.buffer].slice(bv_offset, bv_offset + bv.byteLength); // new buffer
            const blob = new Blob([data], {type: img.mimeType})
            //console.log(blob);
            //return createImageBitmap(blob);
            const image = await createImageBitmap(blob);
            return createImageBitmap(image, {resizeWidth: 1024, resizeHeight: 1024})
        }
        else
        {
            let res = await (img.uri.startsWith("data:")? fetch(img.uri) : fetch(path+img.uri));
            const blob = await res.blob();
            //console.log(blob);
            return createImageBitmap(blob);
        }
    })) : [];
    //console.log('IMAGES', images);

    return await parse_gltf2(gltf, buffers, images);
}

async function parse_gltf2(gltf, buffers, images)
{
    const accessors = gltf.accessors.map(accessor => {
        const view = gltf.bufferViews[accessor.bufferView];
        const buffer = buffers[view.buffer];
        const bv_offset = view.byteOffset? view.byteOffset : 0;
        const bv_length = view.byteLength? view.byteLength : 0;

        const offset = bv_offset + (accessor.byteOffset? accessor.byteOffset : 0);
        const no_components = (accessor.type === 'SCALAR')? 1 : (accessor.type === 'VEC2')? 2 : (accessor.type === 'VEC3')? 3 : (accessor.type === 'VEC4')? 4 : 1;
        const component_size = (accessor.componentType === 5120 || accessor.componentType === 5121)? 1 : (accessor.componentType === 5122 || accessor.componentType === 5123)? 2 : 4;
        const target_byteStride = no_components * component_size;
        let arr = null;
        if(view.byteStride && view.byteStride !== no_components * component_size)
        {
            // strided parse
            const stride = view.byteStride / component_size;
            //console.log("Stride", stride);
            //console.log("Count", accessor.count, no_components);
            const temp_arr = getTypedArray(accessor.componentType, buffer, offset, accessor.count * stride);
            //console.log("S1", temp_arr);
            arr = newTypedArray(accessor.componentType, no_components * accessor.count);
            //console.log("S2", arr);
            for(let i = 0; i < accessor.count; i++)
            {
                for(let j = 0; j < no_components; ++j)
                    arr[i * no_components + j] =  temp_arr[i * stride + j];
            }
            //console.log("S3", arr);
        }
        else
            arr = getTypedArray(accessor.componentType, buffer, offset, no_components * accessor.count);
        //console.log(target_byteStride, view.byteStride, no_components * accessor.count, arr);
        return arr;
    });

    //console.log("Accessors", accessors);

    const cameras = gltf.cameras? gltf.cameras.map(camera => {

        let cam = {
            name: camera.name,
            type: camera.type
        };
        if(camera.type === 'perspective')
        {
            cam.zNear = camera.perspective.znear;
            cam.zFar = camera.perspective.zfar? camera.perspective.zfar : 10000.0;
            cam.fov = camera.perspective.yfov;
            
        }
        else
        {

        }
        
        return cam;
    }) : null;

    const defaultMaterial = {
        name: 'default-material',
        type : MATERIAL_TYPE.GGX, // GGX (Default Material)
        baseColor : [1,1,1,1],
        metallic :  1,
        roughness :  1,
        reflectance : 0.5,
        ior : 1.0,
        baseColorTexture : -1,
        baseColorTextureInfo: null,
        metallicRoughnessTexture : -1,
        metallicRoughnessTextureInfo: null,
        normalsTexture : -1,
        normalsTextureInfo: null,
        occlusionTexture: -1,
        occlusionTextureInfo: null,
        sheenColor: [0,0,0],
        sheenColorTexture: -1,
        sheenColorTextureInfo: null,
        sheenRoughness: 0,
        sheenRoughnessTexture: -1,
        sheenRoughnessTextureInfo: null

    }

    const materials = gltf.materials.map(mat => {

        const alphaMode = mat.alphaMode? toAlphaMode(mat.alphaMode) : toAlphaMode('OPAQUE');
        const emissiveFactor = mat.emissiveFactor? mat.emissiveFactor : null;

        let e_mat = {
            reflectance : 0.5,
            ior : 1.0,
            baseColorTexture : -1,
            baseColorTextureInfo: null,
            metallicRoughnessTexture : -1,
            metallicRoughnessTextureInfo: null,
            normalsTexture : -1,
            normalsTextureInfo: null,
            occlusionTexture: -1,
            occlusionTextureInfo: null,
            sheenColor: [0,0,0],
            sheenColorTexture: -1,
            sheenColorTextureInfo: null,
            sheenRoughness: 0,
            sheenRoughnessTexture: -1,
            sheenRoughnessTextureInfo: null
        }
        
        if(mat.normalTexture)
        {
            if(mat.normalTexture.scale && mat.normalTexture.scale !== 1)
                console.error("We do not support scaling on normals");
            if(mat.normalTexture.texCoord) // if it is not equal to zero
                console.error("We do not support multiple UVs on normal maps");
            const texture_index = gltf.textures[mat.normalTexture.index].source;
            if(typeof texture_index !== 'number')
            {
                console.error("Undefined behaviour: no texture source");
            }
            else
            {
                e_mat.normalsTexture = texture_index;
            }
            // check for extensions (KHR_texture_transform)
            const extensions = mat.normalTexture.extensions;
            e_mat.normalsTextureInfo = getTextureExtensionOffsetScale(extensions);
        }
        if(mat.occlusionTexture)
        {
            if(mat.occlusionTexture.strength && mat.occlusionTexture.strength !== 1)
                console.error("We do not support strenth on occlusion");
            if(mat.occlusionTexture.texCoord) // if it is not equal to zero
                console.error("We do not support multiple UVs on occlusion maps");
            const texture_index = gltf.textures[mat.occlusionTexture.index].source;
            if(typeof texture_index !== 'number')
            {
                console.error("Undefined behaviour: no texture source");
            }
            else
            {
                e_mat.occlusionTexture = texture_index;
            }
            // check for extensions (KHR_texture_transform)
            const extensions = mat.occlusionTexture.extensions;
            e_mat.occlusionTextureInfo = getTextureExtensionOffsetScale(extensions);
        }
        // check for extension (KHR_materials_sheen)
        const extensions = mat.extensions;
        if(extensions)
        {
            const sheen_material_ext = extensions.KHR_materials_sheen;
            if(sheen_material_ext)
            {
                const color = sheen_material_ext.sheenColorFactor? sheen_material_ext.sheenColorFactor : [0,0,0];
                if(Array.isArray(color) && (color[0] !== 0 || color[1] !== 0 || color[2] !== 0))
                {
                    e_mat.sheenColor = color;
                    e_mat.sheenColorTexture = -1;
                    e_mat.sheenColorTextureInfo = null;
                    if(sheen_material_ext.sheenColorTexture)
                    {
                        const sheen_color_texture = sheen_material_ext.sheenColorTexture;
                        e_mat.sheenColorTextureInfo = getTextureExtensionOffsetScale(sheen_color_texture.extensions);
                        const texture_index = gltf.textures[sheen_color_texture.index].source;
                        if(typeof texture_index !== 'number')
                        {
                            console.error("Undefined behaviour: no texture source");
                        }
                        else
                        {
                            e_mat.sheenColorTexture = texture_index;
                        }
                    }

                    e_mat.sheenRoughness = sheen_material_ext.sheenRoughnessFactor? sheen_material_ext.sheenRoughnessFactor : 0;
                    e_mat.sheenRoughnessTexture = -1;
                    e_mat.sheenRoughnessTextureInfo = null;
                    if(sheen_material_ext.sheenRoughnessTexture)
                    {
                        const sheen_roughness_texture = sheen_material_ext.sheenRoughnessTexture;
                        e_mat.sheenRoughnessTextureInfo = getTextureExtensionOffsetScale(sheen_roughness_texture.extensions);
                        const texture_index = gltf.textures[sheen_roughness_texture.index].source;
                        if(typeof texture_index !== 'number')
                        {
                            console.error("Undefined behaviour: no texture source");
                        }
                        else
                        {
                            e_mat.sheenRoughnessTexture = texture_index;
                        }
                    }
                }
            }
        }
        if(mat.pbrMetallicRoughness)
        {
            const pbr = mat.pbrMetallicRoughness;
            e_mat.type = MATERIAL_TYPE.GGX; // GGX
            e_mat.baseColor = pbr.baseColorFactor? pbr.baseColorFactor : [1,1,1,1],
            e_mat.metallic = typeof pbr.metallicFactor !== 'undefined'? pbr.metallicFactor : 1;
            e_mat.roughness = typeof pbr.roughnessFactor !== 'undefined'? pbr.roughnessFactor : 1;

            if(pbr.baseColorTexture) // it is an object
            {
                const texture_index = gltf.textures[pbr.baseColorTexture.index].source;
                if(typeof texture_index !== 'number')
                {
                    console.error("Undefined behaviour: no texture source");
                }
                else
                {
                    e_mat.baseColorTexture = texture_index;
                }
                // check for extensions (KHR_texture_transform)
                const extensions = pbr.baseColorTexture.extensions;
                e_mat.baseColorTextureInfo = getTextureExtensionOffsetScale(extensions);
            }
            if(pbr.metallicRoughnessTexture) // it is an object
            {
                const texture_index = gltf.textures[pbr.metallicRoughnessTexture.index].source;
                if(typeof texture_index !== 'number')
                {
                    console.error("Undefined behaviour: no texture source");
                }
                else
                {
                    e_mat.metallicRoughnessTexture = texture_index;
                }
                // check for extensions (KHR_texture_transform)
                const extensions = pbr.metallicRoughnessTexture.extensions;
                e_mat.metallicRoughnessTextureInfo = getTextureExtensionOffsetScale(extensions);
            }
        }
        else
        {
            e_mat.type = MATERIAL_TYPE.GGX; // GGX (Default Material)
            e_mat.baseColor = [1,1,1,1],
            e_mat.metallic =  1;
            e_mat.roughness =  1;
        }

        return {
            name: mat.name? mat.name : "no-name",
            alphaMode: alphaMode,
            ...e_mat
        };
    });

    materials.push(defaultMaterial);
    const defaultMaterialIndex = materials.length - 1;

    const normalMapping_available = materials.some(e => e.normalsTexture !== -1);
    console.log("Normal Mapping", normalMapping_available);

    const meshes = gltf.meshes.map(mesh => { // DONE

        //const prim_mode = typeof prim.mode !== 'undefined'? 4 : prim.mode;

        // only allow triangles (mode:4)
        const primitives = mesh.primitives.filter(prim => { return typeof prim.mode === 'undefined' || prim.mode === 4 })
        .map(prim => {
            const vertices = accessors[prim.attributes.POSITION]; // vec3
            const indices = typeof prim.indices !== 'undefined'? accessors[prim.indices] : new Uint32Array(vertices.length / 3).map((e,i) => { return i;});
            const normals =  typeof prim.attributes.NORMAL !== 'undefined'? accessors[prim.attributes.NORMAL] : computeSmoothNormals(vertices, indices); // vec3
            const tangents =  typeof prim.attributes.TANGENT !== 'undefined'? accessors[prim.attributes.TANGENT] : null; // vec4
            const texcoords =  typeof prim.attributes.TEXCOORD_0 !== 'undefined'? decodeTexcoords(accessors[prim.attributes.TEXCOORD_0]) : new Float32Array(2 * vertices.length / 3); // vec2

            return {
                indices : indices,
                vertices: vertices, 
                normals: normals,
                //tangents: normalMapping_available? decodeTangents(tangents, vertices.length / 3) : null,
                tangents: normalMapping_available? tangents !== null? decodeTangents4(tangents, vertices.length / 3) : computeTangents(vertices, normals, texcoords, indices) : null,
                texcoords: texcoords,
                material: typeof prim.material !== 'undefined'? prim.material : defaultMaterialIndex
            }
        });

        const num_vertices = primitives.reduce((acc, curr) => acc + curr.vertices.length, 0);
        const num_indices = primitives.reduce((acc, curr) => acc + curr.indices.length, 0);
        const vertices = new Float32Array(num_vertices);
        const normals = new Float32Array(num_vertices);
        const texcoords = new Float32Array(2 * num_vertices / 3);
        const tangents = normalMapping_available? new Float32Array(4 * num_vertices / 3) : null;
        const indices = new Uint32Array(num_indices);
        let prev_vertex_offset = 0;
        let prev_index_offset = 0;
        let parts = [];
        for(let prim of primitives)
        {
            let part = {
                offset: prev_index_offset,
                no_vertices: prim.indices.length,
                material : prim.material
            };
            vertices.set(prim.vertices, prev_vertex_offset);
            normals.set(prim.normals, prev_vertex_offset);
            indices.set(new Uint32Array(prim.indices).map(e => e + prev_vertex_offset / 3), prev_index_offset);
            texcoords.set(prim.texcoords, 2 * prev_vertex_offset / 3);
            if(normalMapping_available)
                tangents.set(prim.tangents, 4 * prev_vertex_offset / 3);
            prev_vertex_offset += prim.vertices.length;
            prev_index_offset += prim.indices.length;

            parts.push(part);
        }
        
        return {
            name: mesh.name? mesh.name : "no-name-mesh",
            vertices : vertices,
            normals : normals,
            texcoords: texcoords,
            tangents: tangents,
            indices: indices,
            parts : parts
        }

    });
    console.log("Meshes", meshes);
    
    let scene_nodes = {
        meshes: [],
        materials: materials,
        cameras: [],
        images: images,
        bbox: {
            minV: [Number.MAX_VALUE,Number.MAX_VALUE,Number.MAX_VALUE],
            maxV: [-Number.MAX_VALUE,-Number.MAX_VALUE,-Number.MAX_VALUE],
        }
    };
    const root_scene_index = gltf.scene? gltf.scene : 0;
    const root_scene = gltf.scenes[root_scene_index];
    const nodes = gltf.nodes.map( node => {
        const name = node.name? node.name : null;
        const children = node.children? node.children : [];
        let matrix;
        if(node.matrix)
            matrix = new Float32Array(node.matrix);
        else
        {
            const T = node.translation? new Float32Array(node.translation) : glMatrix.vec3.fromValues(0,0,0);
            const R = node.rotation? new Float32Array(node.rotation) : glMatrix.quat.create(); // Quaternion
            const S = node.scale? new Float32Array(node.scale) : glMatrix.vec3.fromValues(1,1,1);

            matrix = glMatrix.mat4.fromRotationTranslationScale(glMatrix.mat4.create(), R, T, S); // according to the doc it is T*R*S
        }
        const mesh = typeof node.mesh === 'number'? meshes[node.mesh] : null;
        const camera = typeof node.camera === 'number'? cameras[node.camera] : null;
        // TODO camera?
        return {
            name: name,
            children: children,
            mesh: mesh,
            camera: camera,
            matrix: matrix
        };
    });
    //console.log("Nodes", nodes);
    const traverseNodes = (node, parentMatrix) => {
        //console.log('Traverse', node);

        const matrix = parentMatrix === null? node.matrix : glMatrix.mat4.multiply(glMatrix.mat4.create(), parentMatrix, node.matrix);
        
        if(node.mesh) // mesh is an object
        {
            scene_nodes.meshes.push({
                name: node.name? node.name : node.mesh.name,
                mesh : node.mesh,
                matrix: matrix
            });
            return;
        }
        if(node.camera)
        {
            let cam = {
                name: node.name? node.name : node.camera.name,
                fov: node.camera.fov,
                zNear: node.camera.zNear,
                zFar: node.camera.zFar
            };
            cam.position = glMatrix.vec3.transformMat4(glMatrix.vec3.create(), glMatrix.vec3.fromValues(0,0,0), matrix);
            cam.direction = glMatrix.vec3.transformMat3(glMatrix.vec3.create(), glMatrix.vec3.fromValues(0,0,-1), glMatrix.mat3.fromMat4(glMatrix.mat3.create(), matrix));
            scene_nodes.cameras.push(cam);
            return;
        }
        for(const i of node.children)
        {
            const child = nodes[i];            
            traverseNodes(child, matrix);
        }
    }
    let scale = glMatrix.mat4.scale(glMatrix.mat4.create(), glMatrix.mat4.create(), glMatrix.vec3.fromValues(0.1,0.1,0.1));
    for(let node of root_scene.nodes)
        traverseNodes(nodes[node], null);

    return scene_nodes;
}