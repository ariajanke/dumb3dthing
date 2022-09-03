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

const initShaderProgram = (gl, vsSource, fsSource) => {
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

const setMatrixAndPlatformCallbacks = shaderProgramMap => {
  // platform has a lot of "must sets"
  // but this greatly eases things in the main cpp file
  // afterall the jsPlatform is supposed to take care of everything on the js 
  // side for C++
  jsPlatform.modelMatrix.setApplier((gl, matrix) => {
    gl.uniformMatrix4fv(
      shaderProgramMap.uniformLocations.modelViewMatrix,
      false, matrix);
  });
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
  perspectiveMatrix.setApplier((gl, matrix) => {
    gl.uniformMatrix4fv(
      shaderProgramMap.uniformLocations.projectionMatrix,
      false, matrix);
  });
  jsPlatform.setRenderModelAttributes(
    shaderProgramMap.attribLocations.vertexPosition,
    shaderProgramMap.attribLocations.textureCoord);
  jsPlatform.setTextureUnitHandler((gl, unit) =>
    gl.uniform1i(shaderProgramMap.uniformLocations.uSampler, unit));
  jsPlatform.viewMatrix.setApplier((gl, matrix) => {
    gl.uniformMatrix4fv(
      shaderProgramMap.uniformLocations.viewMatrix,
      false, matrix);
  });
};

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
  
const startUpModule = module => {
  const pressKey_   = module.cwrap('to_js_press_key'  , 'null', ['number']);
  const releaseKey_ = module.cwrap('to_js_release_key', 'null', ['number']);  
  const update_     = module.cwrap('to_js_update', 'null', ['number']);

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
  let mGlContext;
  let mReseter = () => { throw 'perspectiveMatrix.reset: reseter must be set before use.'; };
  let mApplier = () => { throw 'perspectiveMatrix.apply: applier must be set before use.'; };
  return Object.freeze({
    setContext: gl => mGlContext = gl,
    reset: () => blockReturn(mMatrix = mReseter(mGlContext) ),
    // !<needs to be set before start>!
    setReseter: f => blockReturn(mReseter = f),
    apply: () => mApplier(mGlContext, mMatrix),
    // !<needs to be set before start>!
    setApplier: f => blockReturn(mApplier = f)
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

const passToIfDefined = (x, f) => {
  if (typeof x === 'number') f(x);
};

return (() => {
  let mMod;
  let mKeyMapper;
  let mGlContextGetter;
  let mShaderProgram;
  let mGlContext;

  const resetContext = () => {
    mGlContext = mGlContextGetter();
    perspectiveMatrix.setContext(mGlContext);
    jsPlatform.setContext(mGlContext);
  };

  return Object.freeze({
    startUp: (getGlContext, module, keymapper) => {
      mGlContextGetter = getGlContext;
      resetContext();

      mKeyMapper     = keymapper;
      mShaderProgram = startUpWebGlAndGetShader(mGlContext);
      mMod           = startUpModule(module);
      mMod.startUp();
    },
    restart: () => mMod.startUp(),
    update: et => {
      if (mGlContext.isContextLost()) {
        console.log('Attempting to recover WebGL context.');
        resetContext();
      }
      startNewFrame(mGlContext, mShaderProgram);
      mMod.update(et);
    },
    pressKey  : ev => passToIfDefined(mKeyMapper(ev), mMod.pressKey  ),
    releaseKey: ev => passToIfDefined(mKeyMapper(ev), mMod.releaseKey)
  });
})();

})();