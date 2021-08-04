/*
// Types (8) are Directional, OmniPoint, SpotPoint, Sphere, Quad, Triangles, Cylidrical, Box

// color can be usually packed into RGB8 (24bit) (put color into type)

struct DirectionalLight
{
    uchar type; uint24 color; 
    vec3 direction;

    vec3 empty;    
    float intensity;
}

struct OmniPointLight
{
    uchar type; uint24 color; 
    vec3 position;

    vec3 empty;
    float intensity
}

struct SpotPointLight
{
    uchar type; uint24 color; 
    vec3 position;

    ushort phi; ushort theta;
    ushort cone; ushort coneFalloff;
    float intensity;
}

struct SphericalLight
{
    uchar type; uint24 color;
    vec3 position;

    float size;
    vec2 unused
    float intensity;
}

struct QuadLight
{
    uchar type; uint24 emissiveTexture;
    vec3 position;

    half_vec3 tangent;
    half_vec3 bitangent;
    float intensity;
}

*/

// convert from uint32buffer element to uint16buffer element
const float2half = (output, outputIndex, input, inputIndex) => {
    //console.log(input, inputIndex);

    /*output[outputIndex] = 
        ((input[inputIndex] >> 16) & 0x8000) | // sign
        ((((input[inputIndex] & 0x7f800000) - 0x38000000) >> 13) & 0x7c00) | // exponential
        ((input[inputIndex] >> 13) & 0x03ff); // Mantissa        
    console.log(output[outputIndex]);*/
           
    let b = new Int32Array(3); // s, e, m
    b[0] = ((input[inputIndex] >> 16) & 0x00008000);
    b[1] = (((input[inputIndex] >> 23) & 0x000000ff) - (127 - 15));
    b[2] = (input[inputIndex]  & 0x007fffff);
    //console.log(b);

    if(b[1] <= 0)
    {
        if(b[1] < -10)
        {
            // it is a signed zero
            output[outputIndex] = b[0];
            return;
        }
        b[2] = (b[2] | 0x00800000) >> (1 - b[1]);

        if(b[2] & 0x00001000)
            b[2] += 0x00002000;

        output[outputIndex] = (b[0] | (b[2] >> 13));
        return;
    }
    else if(b[1] == 0xff - (127 - 15))
    {
        if(b[2] === 0)
        {
            output[outputIndex] = (s | 0x7c00);
        }
        else
        {
            b[2] >>= 13;
            output[outputIndex] =(b[0] | 0x7c00 | b[2] | (b[2] == 0));
        }
        return;
    }
    else
    {
        if(b[2] & 0x00001000)
        {
            b[2] += 0x00002000;
            if(b[2] & 0x00800000)
            {
                b[2] =  0;     // overflow in significand,
                b[1] += 1;     // adjust exponent
            }
        }
        if (b[1] > 30)
        {
            output[outputIndex] = (b[0] | 0x7c00);
        }
        output[outputIndex] = (b[0] | (b[1] << 10) | (b[2] >> 13));
    }

    //console.log(output[outputIndex]);
}

const half2float = (output, input) => {
    output[0] = ((input & 0x8000) << 16) | ((( input & 0x7c00) + 0x1C000) << 13) | ((input & 0x03FF) << 13);
}

// Types (8) are Directional (1), OmniPoint (2), SpotPoint (3), Sphere (4), Quad (5), Triangles (6), Cylidrical (7), Box (8)
export const LightTypes = {
    DIRECTIONAL: 1,
    OMNI_POINT: 2,
    SPOT_POINT: 3,
    SPHERE: 4,
    QUAD: 5,
    TRIANGLES: 6,
    BOX: 7
};
Object.freeze(LightTypes);

function BaseLight(type, color, intensity)
{
    const color_rgb = color.map(e => e * 255);
    return {
        type: type,
        color: new Uint8ClampedArray(color_rgb),
        intensity: intensity,
        invalid: true,
        shadowmap: null,
        set color(col) {
            this.color[0] = col[0] * 255;
            this.color[1] = col[1] * 255;
            this.color[2] = col[2] * 255;
            this.invalid = true;                        
        },
        set intensity(value) {
            this.intensity = value;
            this.invalid = true; 
        }
    }
}

export function DirectionalLight(name, direction=[1,1,0], color=[1,1,1], intensity=1.0) {

    let dir2 = glMatrix.vec3.fromValues(direction[0], direction[1], direction[2]);
    dir2 = glMatrix.vec3.normalize(dir2, dir2);
    const base = BaseLight(LightTypes.DIRECTIONAL, color, intensity);
    return {
        name: name,
        ...base,
        direction: dir2,
        set direction(dir) {
            let dir2 = glMatrix.vec3.fromValues(dir[0], dir[1], dir[2]);
            dir2 = glMatrix.vec3.normalize(dir2, dir2);
            this.direction = dir2;
            this.invalid = true;
        },
        setAzimuthPolar: function(azimuth, polar)
        {
            const sinAzimuth = Math.sin(azimuth);
            this.direction[0] = Math.cos(polar) * sinAzimuth;
            this.direction[0] = Math.sin(polar) * sinAzimuth;
            this.direction[0] = Math.cos(azimuth);
            this.invalid = true;
        }
    };
}

function SphericalLight(pos=[0,0,0], radius=1.0, color=[1,1,1], intensity=1.0) {

    return {
        type: 4,
        position: glMatrix.vec3.fromValues(pos[0], pos[1], pos[2]),
        radius: radius,
        color: new Uint8ClampedArray(color),
        intensity: intensity
    };
}

function QuadLight(pos=[0,0,0], tangent=[1,0,0], bitangent=[0,1,0], texture = -1, intensity=1.0) {

    return {
        type: 5,
        position: glMatrix.vec3.fromValues(pos[0], pos[1], pos[2]),
        tangent: glMatrix.vec3.fromValues(tangent[0], tangent[1], tangent[2]),
        bitangent: glMatrix.vec3.fromValues(bitangent[0], bitangent[1], bitangent[2]),
        intensity: intensity
    };
}

export function LightManager(renderer) {

    let lightArray = [];
    const gl = renderer.gl;

    return {
        addLight: function(light) {
            lightArray.push(light);
            console.log(lightArray);
        },
        update: function(dt) {
            const invalid = lightArray.some(e => e.invalid);
            if(invalid)
            {
                lightArray.forEach(e => {
                    e.invalid = false;
                });
                
                return true;
            }
            return false;
        },
        getGPUBuffer: function() {
            const vec4SizeBytes = 4 * 4;
            const lightSizeBytes = 2 * vec4SizeBytes;
            let buffer = new ArrayBuffer(Math.max(lightArray.length, 8) * lightSizeBytes);
            let index = 0;
            let floatBuffer = new Float32Array(buffer);
            let uint16Buffer = new Uint16Array(buffer);
            let uint8Buffer = new Uint8Array(buffer);
            for(let light of lightArray)
            {
                console.log('Add ', light);
                if(light.type === LightTypes.DIRECTIONAL)
                {
                    uint8Buffer[lightSizeBytes * index] = light.type; // type 
                    uint8Buffer[lightSizeBytes * index + 1] = light.color[0]; // r
                    uint8Buffer[lightSizeBytes * index + 2] = light.color[1]; // g
                    uint8Buffer[lightSizeBytes * index + 3] = light.color[2]; // b

                    floatBuffer[8 * index + 1] = light.direction[0];
                    floatBuffer[8 * index + 2] = light.direction[1];
                    floatBuffer[8 * index + 3] = light.direction[2];
                
                    // empty 3 floats ...
                    floatBuffer[8 * index + 7] = light.intensity; // intensity
                }
                else if(light.type === 4)
                {
                    uint8Buffer[lightSizeBytes * index] = light.type;
                    uint8Buffer[lightSizeBytes * index + 1] = light.color[0]; // r
                    uint8Buffer[lightSizeBytes * index + 2] = light.color[1]; // g
                    uint8Buffer[lightSizeBytes * index + 3] = light.color[2]; // b

                    floatBuffer[8 * index + 1] = light.position[0];
                    floatBuffer[8 * index + 2] = light.position[1];
                    floatBuffer[8 * index + 3] = light.position[2];
                
                    floatBuffer[8 * index + 4] = light.radius;
                    floatBuffer[8 * index + 7] = light.intensity;
                }
                else if(light.type === 5)
                {
                    uint8Buffer[lightSizeBytes * index] = light.type;
                    uint8Buffer[lightSizeBytes * index + 1] = 0xFF; // r
                    uint8Buffer[lightSizeBytes * index + 2] = 0xFF; // g
                    uint8Buffer[lightSizeBytes * index + 3] = 0xFF; // b

                    floatBuffer[8 * index + 1] = light.position[0];
                    floatBuffer[8 * index + 2] = light.position[1];
                    floatBuffer[8 * index + 3] = light.position[2];

                    const tangentI = new Uint32Array(light.tangent.buffer);
                    const bitangentI = new Uint32Array(light.bitangent.buffer);

                    // Second Vec4
                    float2half(uint16Buffer, 2 * 8 * index + 8 + 0, tangentI, 0);
                    float2half(uint16Buffer, 2 * 8 * index + 8 + 1, tangentI, 1);
                    float2half(uint16Buffer, 2 * 8 * index + 8 + 2, tangentI, 2);

                    float2half(uint16Buffer, 2 * 8 * index + 8 + 3, bitangentI, 0);
                    float2half(uint16Buffer, 2 * 8 * index + 8 + 4, bitangentI, 1);
                    float2half(uint16Buffer, 2 * 8 * index + 8 + 5, bitangentI, 2);
                
                    floatBuffer[8 * index + 7] = light.intensity;
                }

                index += 1;
            }
            console.log(uint8Buffer);
            return floatBuffer;
        }
    }
}
