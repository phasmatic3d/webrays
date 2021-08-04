
const toRadians = degrees => degrees * Math.PI / 180.0;
const toDegrees = radians => 180.0 * radians / Math.PI;

function createPerspectiveCamera(width, height) 
{
    return {
        width: width,
        height: height,
        aspectRatio: width / height,
        fov: 60.0,
        zNear: 0.01,
        zFar: 1000.0,
        projection: glMatrix.mat4.create(),

        setCanvasSize: function(width, height)
        {
            this.width = width;
            this.height = height;
            this.aspectRatio = width / height;
  
            this.recomputePerspectiveMatrix();   
        },
        setNearFar: function(near, far) {
            this.zNear = near;
            this.zFar = far;
            this.recomputePerspectiveMatrix();
        },
        recomputePerspectiveMatrix: function() {            
            this.projection = glMatrix.mat4.perspective(this.projection,
              toRadians(this.fov),
              this.aspectRatio,
              this.zNear, this.zFar);
        },        
    }
}

export function createOrbitCamera(width, height)
{
    // Camera Properties
    const camera_pos = glMatrix.vec3.fromValues(0, 20, 50);
    const camera_target = glMatrix.vec3.fromValues(0, 20, 0);
    const camera_front = glMatrix.vec3.normalize(glMatrix.vec3.create(), glMatrix.vec3.subtract(glMatrix.vec3.create(), camera_target, camera_pos));
    const camera_up = glMatrix.vec3.fromValues(0,1,0);    

    const camera = createPerspectiveCamera(width, height);
    camera.setNearFar(0.01, 10000.0);

    return {
        ...camera,
        camera_pos: camera_pos,
        camera_target: camera_target,
        camera_front: camera_front,
        camera_up: camera_up,
        sceneBBOX: null, // Else {minV: [], maxV: []}
        angles: [Math.atan2(camera_front[2], camera_front[0]), 0], // Radians [azimuth, polar] [-p..p, 0..p]
        zoomLevel: 50.0,
        zoomBounds: [0.01, 1000],
        sceneBBOX: null, // {minValue, maxValue}
        // camera matrices
        view: glMatrix.mat4.create(),
        // Mouse States
        mouse: glMatrix.vec2.fromValues(0, 0),
        last_mouse: glMatrix.vec2.fromValues(0, 0),
        arcball_on: false,
        need_update: true,

        get position() {
            return this.camera_pos;
        },
        set position(xyz) {
            if(!xyz || xyz.length !== 3)
            {
                console.error("Wrong array. Camera");
                return;
            }
            const [x, y, z] = xyz;

            this.camera_pos = glMatrix.vec3.fromValues(x,y,z);
            let camera_n_pos = glMatrix.vec3.sub(glMatrix.vec3.create(), this.camera_pos, this.camera_target);
            this.zoomLevel = glMatrix.vec3.length(camera_n_pos);
            camera_n_pos = glMatrix.vec3.normalize(camera_n_pos, camera_n_pos);

            // Theta azimuth, Phi inclination

            const half_pi = 0.495 * Math.PI;
            const theta = Math.atan2(camera_n_pos[2], camera_n_pos[0]);
            let phi = Math.acos(camera_n_pos[1]);
            phi = Math.abs(camera_n_pos[1]) > 0.99? Math.sign(camera_n_pos[1]) * half_pi : phi;
            
            this.angles = [
              theta,
              phi
            ]; // Radians

            // Recompute Position
            let camera_pos = glMatrix.vec3.fromValues(x,y,z);
            camera_pos[0] = Math.cos(theta) * Math.sin(phi);
            camera_pos[1] = Math.cos(phi);
            camera_pos[2] = Math.sin(theta) * Math.sin(phi);            
            camera_pos = glMatrix.vec3.normalize(camera_pos, camera_pos);
            this.camera_front = glMatrix.vec3.fromValues(-camera_pos[0],-camera_pos[1],-camera_pos[2]);
            camera_pos[0] *= this.zoomLevel;
            camera_pos[1] *= this.zoomLevel;
            camera_pos[2] *= this.zoomLevel;
            this.camera_pos = glMatrix.vec3.add(glMatrix.vec3.create(), camera_pos, this.camera_target);
        
            // The Right vector is on the right of the view vector
            const d = theta - 0.5 * Math.PI;            
            let right = glMatrix.vec3.fromValues(Math.cos(d), 0.0, Math.sin(d));
            right = glMatrix.vec3.normalize(right, right);
            // Compute the up vector
            this.camera_up = glMatrix.vec3.cross(glMatrix.vec3.create(), right, this.camera_front);
            this.camera_up = glMatrix.vec3.normalize(this.camera_up, this.camera_up);
        
            this.recomputeViewMatrix();  
        },
        set target(xyz) {
            if(!xyz || xyz.length !== 3)//if(!Array.isArray(xyz) || xyz.length !== 3)
            {
                console.error("Wrong array. Camera Target");
                return;
            }
            const [x, y, z] = xyz;
            this.camera_target = glMatrix.vec3.fromValues(x,y,z);
            let camera_n_pos = glMatrix.vec3.sub(glMatrix.vec3.create(), this.camera_pos, this.camera_target);
            this.zoomLevel = glMatrix.vec3.length(camera_n_pos);
            camera_n_pos = glMatrix.vec3.normalize(camera_n_pos, camera_n_pos);

            // Theta azimuth, Phi inclination

            const half_pi = 0.495 * Math.PI;
            const theta = Math.atan2(camera_n_pos[2], camera_n_pos[0]);
            let phi = Math.acos(camera_n_pos[1]);
            phi = Math.abs(camera_n_pos[1]) > 0.99? Math.sign(camera_n_pos[1]) * half_pi : phi;
            
            this.angles = [
              theta,
              phi
            ]; // Radians

            // Recompute Position
            let camera_pos = glMatrix.vec3.fromValues(x,y,z);
            camera_pos[0] = Math.cos(theta) * Math.sin(phi);
            camera_pos[1] = Math.cos(phi);
            camera_pos[2] = Math.sin(theta) * Math.sin(phi);            
            camera_pos = glMatrix.vec3.normalize(camera_pos, camera_pos);
            this.camera_front = glMatrix.vec3.fromValues(-camera_pos[0],-camera_pos[1],-camera_pos[2]);
            camera_pos[0] *= this.zoomLevel;
            camera_pos[1] *= this.zoomLevel;
            camera_pos[2] *= this.zoomLevel;
            this.camera_pos = glMatrix.vec3.add(glMatrix.vec3.create(), camera_pos, this.camera_target);
        
            // The Right vector is on the right of the view vector
            const d = theta - 0.5 * Math.PI;            
            let right = glMatrix.vec3.fromValues(Math.cos(d), 0.0, Math.sin(d));
            right = glMatrix.vec3.normalize(right, right);
            // Compute the up vector
            this.camera_up = glMatrix.vec3.cross(glMatrix.vec3.create(), right, this.camera_front);
            this.camera_up = glMatrix.vec3.normalize(this.camera_up, this.camera_up);
        
            this.recomputeViewMatrix();  
        },
        recomputeViewMatrix: function() {
            // View Matrix
            this.view = glMatrix.mat4.lookAt( 
              this.view, // output
              this.camera_pos, // pos
              this.camera_target, // at
              this.camera_up // up
            ); // view is [right, up, forward, -pos]^T;
        
            // Decode Camera View Axis
            this.camera_right = glMatrix.vec3.fromValues( this.view[0], this.view[4], this.view[8] );
            this.camera_up    = glMatrix.vec3.fromValues( this.view[1], this.view[5], this.view[9] );
            this.camera_front = glMatrix.vec3.fromValues( -this.view[2], -this.view[6], -this.view[10] );
            this.need_update  = true;
            this.fitNearFar();
        },
        setSceneBBOX: function(bbox)
        {
            this.sceneBBOX = bbox;
            this.fitNearFar();
        },
        fitNearFar: function() {
            //console.log("Fit", this.sceneBBOX);
            if(this.sceneBBOX === null)
                return;
            const scene_size = [this.sceneBBOX.maxValue[0] - this.sceneBBOX.minValue[0], this.sceneBBOX.maxValue[1] - this.sceneBBOX.minValue[1], this.sceneBBOX.maxValue[2] - this.sceneBBOX.minValue[2]];
            const scene_centerA = [
                0.5 * (this.sceneBBOX.maxValue[0] + this.sceneBBOX.minValue[0]), 
                0.5 * (this.sceneBBOX.maxValue[1] + this.sceneBBOX.minValue[1]), 
                0.5 * (this.sceneBBOX.maxValue[2] + this.sceneBBOX.minValue[2])
            ];
            const scene_center = glMatrix.vec3.fromValues(scene_centerA[0], scene_centerA[1], scene_centerA[2]);
            const scene_radius = 0.5 * Math.sqrt(scene_size[0] * scene_size[0] + scene_size[1] * scene_size[1] + scene_size[2] * scene_size[2]);
            const scene_max_side = Math.max(...scene_size);
            const scene_min_side = Math.min(...scene_size);
            const units_scale = 0.01; // 1 cm

            //console.log("Scene Min", scene_min_side);

            let new_near = 0.013; // Put the 1% of BBOX
            let new_far = 133;

            const {minValue, maxValue} = this.sceneBBOX;

            let corner = glMatrix.vec3.create();
            let center_ecs = glMatrix.vec3.transformMat4(glMatrix.vec3.create(), this.camera_pos, this.view);

		    let cur_far = -Number.MAX_VALUE;
		    let cur_near = Number.MAX_VALUE;
            let changed = false;

            for (let i = 0; i < 8; ++i)
		    {
                corner = glMatrix.vec3.fromValues(i & 1 ? maxValue[0] : minValue[0], i & 2 ? maxValue[1] : minValue[1], i & 4 ? maxValue[2] : minValue[2]);
                corner = glMatrix.vec3.transformMat4(glMatrix.vec3.create(), corner, this.view);
      			
			    if (corner[2] >= 0.0)
			    {
				    cur_near = units_scale;
				    continue;
			    }

			    let f = -corner[2];
			    if (f < cur_near)
			    {
    				cur_near = f;
				    changed = true;
			    }
			    if (f > cur_far)
			    {
    				cur_far = f;
				    changed = true;
                }
            }
            //console.log("Near/Far", cur_near, cur_far, changed);

		    let center_box_ecs = glMatrix.vec3.transformMat4(glMatrix.vec3.create(), scene_center, this.view);
		    let dist_to_bbox_center = glMatrix.vec3.distance(center_ecs, center_box_ecs);
		    let inside_bounding_sphere = dist_to_bbox_center <= scene_radius;

		    if (inside_bounding_sphere)
		    {
                //console.log("Inside Bounding Sphere");
			    new_near = units_scale;
			    new_far = new_near + cur_far;
		    }
		    else
		    {
                //console.log("Outside Bounding Sphere");
			    if (changed)
			    {
    				new_near = Math.max(units_scale, cur_near);
	    			new_far = new_near + cur_far;
			    }
			    else
			    {
    				// if we are outside the bounding sphere looking at nothing, just set a default near far
				    new_near = units_scale;
				    new_far = new_near + 1.0;
    			}
            }
            
            new_near -= 0.1 + 0.1 * scene_max_side;//getSceneBox().getMaxSide();
	        new_near = Math.max(new_near, units_scale /*unit scales*/);

            new_far += 0.1;
            console.log("Near/Far :", new_near, new_far);
            this.setNearFar(new_near, new_far);
        },
        mouseMove: function(mouse)
        {
            if (this.arcball_on) {
                mouse.target.style.cursor = 'move';
                this.mouse = glMatrix.vec2.fromValues(mouse.offsetX, mouse.offsetY);
            }
        },
        mouseUp: function(mouse) 
        {
            switch( mouse.button ) {
            case 0:
                if (this.arcball_on)
                    mouse.target.style.cursor = 'pointer';
                this.arcball_on = false;
            break;
            case 1:
            break;
            case 2:
            break;  
            default:
            break;
            }
        },
        mouseDown: function(mouse)
        {
            switch( mouse.button ) {
                case 0:
                this.arcball_on = true;
                this.mouse = this.last_mouse = glMatrix.vec2.fromValues(mouse.offsetX, mouse.offsetY);
                break;
                case 1:
                break;
                case 2:
                break;
                default:
                break;
            }
        },
        touchStart: function(touch)
        {
            this.arcball_on = true;
            this.mouse = this.last_mouse = glMatrix.vec2.fromValues(touch.touches[0].clientX, touch.touches[0].clientY);
        },
        touchEnd: function(touch) {
            this.arcball_on = false;
        },
        touchMove: function(touch) {
            if (this.arcball_on) {
                this.mouse = glMatrix.vec2.fromValues(touch.touches[0].clientX, touch.touches[0].clientY);
            }
        },
        wheelScroll: function(event) {      
            // ZoomIn/Out always 1.2 or 0.83 of the current zoom level
            let zoom_multiplier = (event.deltaY === 0)? 1.0 : event.deltaY > 0? 1.0 / 1.2 : 1.2;
            this.zoomLevel *= zoom_multiplier;
            //console.log("Zoom", zoom_multiplier, this.height, this.zoomLevel);
            //console.log("Pos", this.camera_pos);
            this.zoomLevel = Math.min(this.zoomBounds[1], Math.max(this.zoomLevel, this.zoomBounds[0]));
  
            this.camera_pos[0] = -this.zoomLevel;
            this.camera_pos[1] = -this.zoomLevel;
            this.camera_pos[2] = -this.zoomLevel;
            this.camera_pos = glMatrix.vec3.multiply(glMatrix.vec3.create(), this.camera_pos, this.camera_front);
            this.camera_pos = glMatrix.vec3.add(glMatrix.vec3.create(), this.camera_pos, this.camera_target);
    
            this.recomputeViewMatrix();
            event.stopPropagation();
            event.preventDefault();
        },
        update: function() {
            const ret_value   = this.need_update;
            this.need_update  = false;
            if (this.mouse[0] == this.last_mouse[0] && 
                this.mouse[1] == this.last_mouse[1])
            return ret_value;
          
            var delta = glMatrix.vec2.fromValues(1, -1);
            const mouse_delta = glMatrix.vec2.sub(glMatrix.vec2.create(), this.last_mouse, this.mouse);
            delta = glMatrix.vec2.mul(glMatrix.vec2.create(), delta, mouse_delta);
            delta[0] *= 0.004; // NEED to multiply with dt
            delta[1] *= 0.002;// NEED to multiply with dt
      
            this.last_mouse[0] = this.mouse[0];
            this.last_mouse[1] = this.mouse[1];
      
            //console.log("Start");
    
            this.angles[0] -= delta[0]; // X axis 
            this.angles[1] -= delta[1]; // Y axis 
        
            const halfPI = Math.PI * 0.495;
            const smallBias = 0.005 * Math.PI;
            //this.angles[1] = Math.max(Math.min(this.angles[1], halfPI), -halfPI);
            this.angles[1] = Math.max(Math.min(this.angles[1], Math.PI - smallBias), 0 + smallBias);
        
            // Recompute Position
            const theta = this.angles[0];
            const phi = this.angles[1];
            let camera_pos = glMatrix.vec3.fromValues(0,0,0);
            camera_pos[0] = Math.cos(theta) * Math.sin(phi);
            camera_pos[1] = Math.cos(phi);
            camera_pos[2] = Math.sin(theta) * Math.sin(phi);            
            camera_pos = glMatrix.vec3.normalize(camera_pos, camera_pos);
            this.camera_front = glMatrix.vec3.fromValues(-camera_pos[0],-camera_pos[1],-camera_pos[2]);
            camera_pos[0] *= this.zoomLevel;
            camera_pos[1] *= this.zoomLevel;
            camera_pos[2] *= this.zoomLevel;
            this.camera_pos = glMatrix.vec3.add(glMatrix.vec3.create(), camera_pos, this.camera_target);
    
            // The Right vector is on the right of the view vector
            const d = theta - 0.5 * Math.PI;            
            let right = glMatrix.vec3.fromValues(Math.cos(d), 0.0, Math.sin(d));
            right = glMatrix.vec3.normalize(right, right);
            // Compute the up vector
            this.camera_up = glMatrix.vec3.cross(glMatrix.vec3.create(), right, this.camera_front);
            this.camera_up = glMatrix.vec3.normalize(this.camera_up, this.camera_up);
    
            this.recomputeViewMatrix();  
    
            //WebRaysViewer.last_mouse = WebRaysViewer.mouse;
            return true;
        },
        attachControls: function(canvas) {
            const call_during_capture = true;
            canvas.addEventListener("mousemove", (ev) => this.mouseMove(ev), call_during_capture);
            canvas.addEventListener("mouseup", (ev) => {this.mouseUp(ev);}, call_during_capture);
            canvas.addEventListener("mousedown", (ev) => {this.mouseDown(ev);}, call_during_capture);
            canvas.addEventListener("touchstart", ev => {this.touchStart(ev);}, {passive: true});
            canvas.addEventListener("touchend", (ev) => {this.touchEnd(ev);}, {passive: true});
            //canvas.addEventListener("touchcancel", handleCancel, false);
            canvas.addEventListener("touchmove", (ev) => {this.touchMove(ev);}, {passive: true});
            canvas.addEventListener('wheel', (ev) => {this.wheelScroll(ev);}, call_during_capture); // {passive: true}
        }
    };
}

export function createOrbitCameraOLD(width, height)
{
    // Camera Properties
    const camera_pos = glMatrix.vec3.fromValues(0, 20, 50);
    const camera_target = glMatrix.vec3.fromValues(0, 20, 0);
    const camera_front = glMatrix.vec3.normalize(glMatrix.vec3.create(), glMatrix.vec3.subtract(glMatrix.vec3.create(), camera_target, camera_pos));
    const camera_up = glMatrix.vec3.fromValues(0,1,0);    

    const camera = createPerspectiveCamera(width, height);

    return {
        ...camera,
        camera_pos: camera_pos,
        camera_target: camera_target,
        camera_front: camera_front,
        camera_up: camera_up,
        sceneBBOX: null, // Else {minV: [], maxV: []}
        angles: [Math.atan2(camera_front[2], camera_front[0]), 0], // Radians
        zoomLevel: 50.0,
        zoomBounds: [0.1, 1000],
        // camera matrices
        view: glMatrix.mat4.create(),
        projection: glMatrix.mat4.perspective(glMatrix.mat4.create(),
            60.0 * Math.PI / 180,
            4.0/3,
            0.1, 10000.0),
        // Mouse States
        mouse: glMatrix.vec2.fromValues(0, 0),
        last_mouse: glMatrix.vec2.fromValues(0, 0),
        arcball_on: false,
        need_update: true,

        get position() {
            return this.camera_pos;
        },
        set position(xyz) {
            if(!xyz || xyz.length !== 3)
            {
                console.error("Wrong array. Camera");
                return;
            }
            const [x, y, z] = xyz;
            this.camera_pos = glMatrix.vec3.fromValues(x,y,z);
            this.camera_front = glMatrix.vec3.sub(glMatrix.vec3.create(), this.camera_target, this.camera_pos);
            this.zoomLevel = glMatrix.vec3.length(this.camera_front);
            this.camera_front = glMatrix.vec3.normalize(this.camera_front, this.camera_front);
        
            const half_pi = 0.5 * Math.PI;
            const d2 = Math.abs(this.camera_front[1]) > 0.99? 
              Math.sign(this.camera_front[1]) * half_pi : 
              Math.sqrt(this.camera_front[0] * this.camera_front[0] + this.camera_front[2] * this.camera_front[2]);
                this.angles = [
              Math.atan2(this.camera_front[2], this.camera_front[0]), 
              -Math.atan(this.camera_front[1] / d2)
            ]; // Radians
        
            // The Right vector is on the right of the view vector
            const theta = this.angles[1];
            const phi = -this.angles[0];
              const d = phi - 0.5 * Math.PI;
              let right = glMatrix.vec3.fromValues(Math.cos(d), 0.0, Math.sin(d));
            right = glMatrix.vec3.normalize(right, right);
            // Compute the up vector
            this.camera_up = glMatrix.vec3.cross(glMatrix.vec3.create(), right, this.camera_front);
            this.camera_up = glMatrix.vec3.normalize(this.camera_up, this.camera_up);
        
            this.recomputeViewMatrix();  
        },
        set target(xyz) {
            if(!Array.isArray(xyz) || xyz.length !== 3)
            {
                console.error("Wrong array. Camera");
                return;
            }
            const [x, y, z] = xyz;
            this.camera_target = glMatrix.vec3.fromValues(x,y,z);
            this.camera_front = glMatrix.vec3.sub(glMatrix.vec3.create(), this.camera_target, this.camera_pos);
            this.camera_front = glMatrix.vec3.normalize(this.camera_front, this.camera_front);
        
            const half_pi = 0.5 * Math.PI;
            const d2 = Math.abs(this.camera_front[1]) > 0.99? 
              Math.sign(this.camera_front[1]) * half_pi : 
              Math.sqrt(this.camera_front[0] * this.camera_front[0] + this.camera_front[2] * this.camera_front[2]);
                this.angles = [
              Math.atan2(this.camera_front[2], this.camera_front[0]), 
              -Math.atan(this.camera_front[1] / d2)
            ]; // Radians
        
            // The Right vector is on the right of the view vector
            const theta = this.angles[1];
            const phi = -this.angles[0];
            const d = phi - 0.5 * Math.PI;
            let right = glMatrix.vec3.fromValues(Math.cos(d), 0.0, Math.sin(d));
            right = glMatrix.vec3.normalize(right, right);
            // Compute the up vector
            this.camera_up = glMatrix.vec3.cross(glMatrix.vec3.create(), right, this.camera_front);
            this.camera_up = glMatrix.vec3.normalize(this.camera_up, this.camera_up);
        
            this.recomputeViewMatrix();
        },
        recomputeViewMatrix: function() {
            // View Matrix
            this.view = glMatrix.mat4.lookAt( 
              this.view, // output
              this.camera_pos, // pos
              this.camera_target, // at
              this.camera_up // up
            ); // view is [right, up, forward, -pos]^T;
        
            // Decode Camera View Axis
            this.camera_right = glMatrix.vec3.fromValues( this.view[0], this.view[4], this.view[8] );
            this.camera_up    = glMatrix.vec3.fromValues( this.view[1], this.view[5], this.view[9] );
            this.camera_front = glMatrix.vec3.fromValues( -this.view[2], -this.view[6], -this.view[10] );
            this.need_update  = true;
        },
        mouseMove: function(mouse)
        {
            if (this.arcball_on) {
                mouse.target.style.cursor = 'move';
                this.mouse = glMatrix.vec2.fromValues(mouse.offsetX, mouse.offsetY);
            }
        },
        mouseUp: function(mouse) 
        {
            switch( mouse.button ) {
            case 0:
                if (this.arcball_on)
                    mouse.target.style.cursor = 'pointer';
                this.arcball_on = false;
            break;
            case 1:
            break;
            case 2:
            break;  
            default:
            break;
            }
        },
        mouseDown: function(mouse)
        {
            switch( mouse.button ) {
                case 0:
                this.arcball_on = true;
                this.mouse = this.last_mouse = glMatrix.vec2.fromValues(mouse.offsetX, mouse.offsetY);
                break;
                case 1:
                break;
                case 2:
                break;
                default:
                break;
            }
        },
        touchStart: function(touch)
        {
            this.arcball_on = true;
            this.mouse = this.last_mouse = glMatrix.vec2.fromValues(touch.touches[0].clientX, touch.touches[0].clientY);
        },
        touchEnd: function(touch) {
            this.arcball_on = false;
        },
        touchMove: function(touch) {
            if (this.arcball_on) {
                this.mouse = glMatrix.vec2.fromValues(touch.touches[0].clientX, touch.touches[0].clientY);
            }
        },
        wheelScroll: function(event) {      
            // ZoomIn/Out always 1.2 or 0.83 of the current zoom level
            let zoom_multiplier = (event.deltaY === 0)? 1.0 : event.deltaY > 0? 1.0 / 1.2 : 1.2;
            this.zoomLevel *= zoom_multiplier;
            console.log("Zoom", zoom_multiplier, this.height, this.zoomLevel);
            this.zoomLevel = Math.min(this.zoomBounds[1], Math.max(this.zoomLevel, this.zoomBounds[0]));
  
            this.camera_pos[0] = -this.zoomLevel;
            this.camera_pos[1] = -this.zoomLevel;
            this.camera_pos[2] = -this.zoomLevel;
            this.camera_pos = glMatrix.vec3.multiply(glMatrix.vec3.create(), this.camera_pos, this.camera_front);
            this.camera_pos = glMatrix.vec3.add(glMatrix.vec3.create(), this.camera_pos, this.camera_target);
    
            this.recomputeViewMatrix();
            event.stopPropagation();
            event.preventDefault();
        },
    update: function() {
        const ret_value   = this.need_update;
        this.need_update  = false;
        if (this.mouse[0] == this.last_mouse[0] && 
            this.mouse[1] == this.last_mouse[1])
          return ret_value;
          
        var delta = glMatrix.vec2.fromValues(1, -1);
        const mouse_delta = glMatrix.vec2.sub(glMatrix.vec2.create(), this.last_mouse, this.mouse);
        delta = glMatrix.vec2.mul(glMatrix.vec2.create(), delta, mouse_delta);
        delta[0] *= 0.004; // NEED to multiply with dt
        delta[1] *= 0.002;// NEED to multiply with dt
      
        this.last_mouse[0] = this.mouse[0];
        this.last_mouse[1] = this.mouse[1];
      
        //console.log("Start");
    
        this.angles[0] += delta[0]; // X axis 
        this.angles[1] += delta[1]; // Y axis 
        
        const halfPI = Math.PI * 0.495;
        this.angles[1] = Math.max(Math.min(this.angles[1], halfPI), -halfPI);          
        
        let camera_pos = glMatrix.vec3.create();    
        // Spherical Coordinates
        const theta = this.angles[1];
        const phi = -this.angles[0];
        camera_pos[0] = Math.cos(theta) * Math.cos(phi);
          camera_pos[1] = Math.sin(theta);
          camera_pos[2] = Math.cos(theta) * Math.sin(phi);
        camera_pos = glMatrix.vec3.normalize(camera_pos, camera_pos);
        // The Right vector is on the right of the view vector
          const d = phi + 0.5 * Math.PI;
          let right = glMatrix.vec3.fromValues(Math.cos(d), 0.0, Math.sin(d));
        right = glMatrix.vec3.normalize(right, right);
        // Compute the up vector
          this.camera_up = glMatrix.vec3.cross(glMatrix.vec3.create(), right, camera_pos);
        this.camera_up = glMatrix.vec3.normalize(this.camera_up, this.camera_up);
        
        camera_pos[0] *= this.zoomLevel;
        camera_pos[1] *= this.zoomLevel;
        camera_pos[2] *= this.zoomLevel;
        
        this.camera_pos = glMatrix.vec3.add(glMatrix.vec3.create(), camera_pos, this.camera_target);
        this.view = glMatrix.mat4.lookAt( 
          glMatrix.mat4.create(),
          this.camera_pos, // pos
          this.camera_target, // at
          this.camera_up // up
        ); // view is [right, up, -forward, -pos]^T;
        
        this.camera_right = glMatrix.vec3.fromValues( this.view[0], this.view[4], this.view[8] );
        this.camera_up = glMatrix.vec3.fromValues( this.view[1], this.view[5], this.view[9] );
        this.camera_front = glMatrix.vec3.fromValues( -this.view[2], -this.view[6], -this.view[10] );
    
        //WebRaysViewer.last_mouse = WebRaysViewer.mouse;
        return true;
    },
    attachControls: function(canvas) {
        const call_during_capture = true;
        canvas.addEventListener("mousemove", (ev) => this.mouseMove(ev), call_during_capture);
        canvas.addEventListener("mouseup", (ev) => {this.mouseUp(ev);}, call_during_capture);
        canvas.addEventListener("mousedown", (ev) => {this.mouseDown(ev);}, call_during_capture);
        canvas.addEventListener("touchstart", ev => {this.touchStart(ev);}, {passive: true});
        canvas.addEventListener("touchend", (ev) => {this.touchEnd(ev);}, {passive: true});
        //canvas.addEventListener("touchcancel", handleCancel, false);
        canvas.addEventListener("touchmove", (ev) => {this.touchMove(ev);}, {passive: true});
        canvas.addEventListener('wheel', (ev) => {this.wheelScroll(ev);}, call_during_capture); // {passive: true}
    }
};
}

