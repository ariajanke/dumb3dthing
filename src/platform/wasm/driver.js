/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************/

const driver = (() => {
"use strict";

const kBuiltinVertexShaderSource = `
attribute vec4 aVertexPosition;
attribute vec2 aTextureCoord;

uniform mat4 uModelViewMatrix;
uniform mat4 uViewMatrix; // <- added by me
uniform mat4 uProjectionMatrix;

varying highp vec2 vTextureCoord;
void main(void) {
  gl_Position = uProjectionMatrix * uViewMatrix * uModelViewMatrix * aVertexPosition;
  vTextureCoord = aTextureCoord;
}
`;

const kBuiltinFragmentShaderSource = `
varying highp vec2 vTextureCoord;
uniform sampler2D uSampler;
void main(void) {
  gl_FragColor = texture2D(uSampler, vTextureCoord);
}
`;

const initShaderProgram = (gl, vsSource, fsSource) => {
  const loadShader = (gl, type, source) => {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      // already at a fork of sorts... should I use helper libraries
      // so I can use monads and things like that?
      console.log(`Failed to compile shader: ${gl.getShaderInfoLog(shader)}`);
      gl.deleteShader(shader);
      return null; 
    }
    return shader;
  };

  const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
  const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

  const shaderProgram = gl.createProgram();
  gl.attachShader(shaderProgram, vertexShader);
  gl.attachShader(shaderProgram, fragmentShader);
  gl.linkProgram(shaderProgram);

  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    console.log(`Unable to initialize the shader program: ${gl.getProgramInfoLog(shaderProgram)}`);
    return null;
  }

  return shaderProgram;
};

// should only call this once...
const setPlatformCallbacks = getShader => {
  // shader matricies
  // no good... requries shader attributes
  jsPlatform.modelMatrix.setApplierFactory(gl => {
    // get attributes here... okay...
    // what about an up-to-date shader?
    // driver knows how to get the shader for this context
    const shaderProgram = getShader();
    const attrLoc       = gl.getUniformLocation(shaderProgram, "uModelViewMatrix");
    return matrix => gl.uniformMatrix4fv(attrLoc, false, matrix);
  });

  jsPlatform.viewMatrix.setApplierFactory(gl => {
    const shaderProgram = getShader();
    const attrLoc       = gl.getUniformLocation(shaderProgram, "uViewMatrix")
    return matrix => gl.uniformMatrix4fv(attrLoc, false, matrix);
  });
  
  jsPlatform.setRenderModelAttributesNeederFactory(gl => {
    const shaderProgram  = getShader();
    const vertexPosition = gl.getAttribLocation(shaderProgram, "aVertexPosition");
    const textureCoord   = gl.getAttribLocation(shaderProgram, "aTextureCoord");
    return render => render(vertexPosition, textureCoord);
  });
  
  jsPlatform.setTextureUnitHandlerFactory(gl => {
    const shaderProgram = getShader();
    const samplerAttrLoc = gl.getUniformLocation(shaderProgram, "uSampler");
    return unit => gl.uniform1i(samplerAttrLoc, unit);
  });

  perspectiveMatrix.setReseterFactory(gl => {
    const aspect = gl.canvas.clientWidth / gl.canvas.clientHeight;
    return () => {
      const fieldOfView = 45 * Math.PI / 180;   // in radians
      const zNear = 0.1;
      const zFar = 100.0;
      const projectionMatrix = mat4.create();
  
      // note: glmatrix.js always has the first argument
      // as the destination to receive the result.
      mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);
      return projectionMatrix;
    };
  });

  perspectiveMatrix.setApplierFactory(gl => {
    const projectionMatrix = gl.getUniformLocation(getShader(), "uProjectionMatrix");
    return matrix => gl.uniformMatrix4fv(projectionMatrix, false, matrix);
  });
};

if (false) {
const setMatrixAndPlatformCallbacks = shaderProgramMap => {
  // platform has a lot of "must sets"
  // but this greatly eases things in the main cpp file
  // afterall the jsPlatform is supposed to take care of everything on the js 
  // side for C++
  if (false)
  jsPlatform.modelMatrix.setApplier((gl, matrix) => {
    gl.uniformMatrix4fv(
      shaderProgramMap.uniformLocations.modelViewMatrix,
      false, matrix);
  });
  if (false)
  perspectiveMatrix.setReseter(gl => {
    const fieldOfView = 45 * Math.PI / 180;   // in radians
    const aspect = gl.canvas.clientWidth / gl.canvas.clientHeight;
    const zNear = 0.1;
    const zFar = 100.0;
    const projectionMatrix = mat4.create();

    // note: glmatrix.js always has the first argument
    // as the destination to receive the result.
    mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);
    return projectionMatrix;
  });
  if (false)
  perspectiveMatrix.setApplier((gl, matrix) => {
    gl.uniformMatrix4fv(
      shaderProgramMap.uniformLocations.projectionMatrix,
      false, matrix);
  });
  if (false)
  jsPlatform.setRenderModelAttributes(
    shaderProgramMap.attribLocations.vertexPosition,
    shaderProgramMap.attribLocations.textureCoord);
  if (false)
  jsPlatform.setTextureUnitHandler((gl, unit) =>
    gl.uniform1i(shaderProgramMap.uniformLocations.uSampler, unit));
  if (false)
  jsPlatform.viewMatrix.setApplier((gl, matrix) => {
    gl.uniformMatrix4fv(
      shaderProgramMap.uniformLocations.viewMatrix,
      false, matrix);
  });
};
}
if (false) {
const startUpWebGlAndGetShader = initGl => {
  const shaderProgram = initShaderProgram(initGl, 
    kBuiltinVertexShaderSource, kBuiltinFragmentShaderSource);

  const programInfo = {
    program: shaderProgram,
    attribLocations: {
      vertexPosition: initGl.getAttribLocation(shaderProgram, "aVertexPosition"),
      textureCoord: initGl.getAttribLocation(shaderProgram, "aTextureCoord"),
    },
    uniformLocations: {
      projectionMatrix: initGl.getUniformLocation(
        shaderProgram,
        "uProjectionMatrix"
      ),
      modelViewMatrix: initGl.getUniformLocation(shaderProgram, "uModelViewMatrix"),
      uSampler: initGl.getUniformLocation(shaderProgram, "uSampler"),
      viewMatrix: initGl.getUniformLocation(shaderProgram, "uViewMatrix")
    }
  };
  
  setMatrixAndPlatformCallbacks(programInfo);
  return programInfo.program;
};
  
}
const startUpModule = module => {
  const pressKey_   = module.cwrap('to_js_press_key'  , 'null', ['number']);
  const releaseKey_ = module.cwrap('to_js_release_key', 'null', ['number']);  
  const update_     = module.cwrap('to_js_update'     , 'null', ['number']);

  return Object.freeze({
    startUp   : module.cwrap('to_js_start_up', 'null'),
    pressKey  : pressKey_,
    releaseKey: releaseKey_,
    update    : update_
  });

  return Object.freeze({
    startUp   : module.cwrap('to_js_start_up'   , 'null'),
    pressKey  : ev => (ev = keymapper(ev)) ? pressKey_  (ev - 1) : undefined,
    releaseKey: ev => (ev = keymapper(ev)) ? releaseKey_(ev - 1) : undefined,
    update    : et => {
      // I should get a reset function from the page itself

      // document.querySelector('#canvas').getContext('webgl')

      drawScene(gl, shaderProgram, () => update_(et));
    }
  });
};

const perspectiveMatrix = (() => {
  const blockReturn = () => {};
  let mMatrix;
  //let mGlContext;
  let mReseter = () => { throw 'perspectiveMatrix.reset: reseter must be set before use.'; };
  let mApplier = () => { throw 'perspectiveMatrix.apply: applier must be set before use.'; };
  let mReseterFactory = () => { throw ''; };
  let mApplierFactory = () => { throw ''; };
  return Object.freeze({
    setContext: gl => {
      mApplier = mApplierFactory(gl);
      mReseter = mReseterFactory(gl);
    },
    reset: () => blockReturn(mMatrix = mReseter() ),
    // !<needs to be set before start>!
    setReseterFactory: f => blockReturn(mReseterFactory = f),
    apply: () => mApplier(mMatrix),
    // !<needs to be set before start>!
    setApplierFactory: f => blockReturn(mApplierFactory = f)
  });
})();

if (false) {
const startUp = (gl, module, keymapper) => {
  const shaderProgram = startUpWebGl(gl);
  const rv = startUpModule(gl, module, keymapper, shaderProgram);
  rv.startUp();
  return rv;
};
}

const startNewFrame = (gl, shaderProgram) => {
  gl.clearColor(0.0, 0.0, 0.0, 1.0);  // Clear to black, fully opaque
  gl.clearDepth(1.0);                 // Clear everything
  gl.enable(gl.DEPTH_TEST);           // Enable depth testing
  gl.depthFunc(gl.LEQUAL);            // Near things obscure far things

  // Clear the canvas before we start drawing on it.

  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  // Create a perspective matrix, a special matrix that is
  // used to simulate the distortion of perspective in a camera.
  // Our field of view is 45 degrees, with a width/height
  // ratio that matches the display size of the canvas
  // and we only want to see objects between 0.1 units
  // and 100 units away from the camera.

  perspectiveMatrix.reset();
  
  // Tell WebGL to use our program when drawing
  gl.useProgram(shaderProgram);

  perspectiveMatrix.apply();
};
if (false) {
const passToIfDefined = (x, f) => {
  if (typeof x === 'number') f(x);
};
}
const makePassToIfNumber = f => ev => {
  const x = f(ev);
  if (typeof x === 'number') f(x);
};

return (() => {
  let mMod;
  let mKeyMapper;
  let mGlContextGetter = () => { throw ''; };
  let mShaderProgram;
  let mGlContext;
  let mTargetFrameRate = 48; // I seem to like to set this a lot...
  let mHalt = false;

  const resetContext = () => {
    mGlContext = mGlContextGetter();
    mShaderProgram = initShaderProgram(mGlContext,
      kBuiltinVertexShaderSource, kBuiltinFragmentShaderSource);
    perspectiveMatrix.setContext(mGlContext);
    jsPlatform.setContext(mGlContext);
    if (false) mShaderProgram = startUpWebGlAndGetShader(mGlContext);
  };

  const pressKey_ = makePassToIfNumber(ev => mMod.pressKey(ev));
  const releaseKey_ = makePassToIfNumber(ev => mMod.releaseKey(ev));

  const makeUpdateRoutine = () => {
    let oldFrameRate = mTargetFrameRate;
    const thisUpdater = () => {
      // get here after browser yield
      if (mHalt) {
        console.log('Update routine stopped.');
        return; // <- stop, no more udpates!
      }
      startNewFrame(mGlContext, mShaderProgram);
      mMod.update(1 / oldFrameRate);
      if (oldFrameRate != mTargetFrameRate) {
        return setTimeout(makeUpdateRoutine(), (1000 / mTargetFrameRate));
      }
      setTimeout(thisUpdater, 1000 / oldFrameRate);
    };
    return thisUpdater;
  };

  return Object.freeze({
    // call once per page load
    startUp: (getGlContext, module, keymapper) => {
      setPlatformCallbacks(() => mShaderProgram);
      mGlContextGetter = getGlContext;
      resetContext();
      mKeyMapper = keymapper;
      mMod       = startUpModule(module);
      mMod.startUp();
      
      setTimeout(makeUpdateRoutine(), (1000 / mTargetFrameRate));
    },
    setTargetFrameRate: fps => mTargetFrameRate = fps,
    targetFrameRate: () => mTargetFrameRate,
    restart: () => mMod.startUp(),
    onContextRestore: () => {
      console.log('Attempting to restore context');
      resetContext();
      setTimeout(makeUpdateRoutine(), (1000 / mTargetFrameRate));
    },
    onContextLost: () => {
      mHalt = true;
    },
    pressKey  : ev => pressKey_(mKeyMapper(ev)),
    releaseKey: ev => releaseKey_(mKeyMapper(ev))
  });
})();

})();