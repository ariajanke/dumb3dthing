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

const startUpModule = module => Object.freeze({
  startUp   : module.cwrap('to_js_start_up'   , 'null'),
  pressKey  : module.cwrap('to_js_press_key'  , 'null', ['number']),
  releaseKey: module.cwrap('to_js_release_key', 'null', ['number']),
  update    : module.cwrap('to_js_update'     , 'null', ['number'])
});


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

const startNewFrame = (gl, shaderProgram) => {
  gl.clearColor(0.1, 0.2, 0.4, 1.0);
  gl.clearDepth(1.0);
  gl.enable(gl.DEPTH_TEST);
  gl.depthFunc(gl.LEQUAL);

  // Clear the canvas before we start drawing on it.

  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  perspectiveMatrix.reset();
  
  // Tell WebGL to use our program when drawing
  gl.useProgram(shaderProgram);

  perspectiveMatrix.apply();
};

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
  };

  const pressKey_   = makePassToIfNumber(ev => mMod.pressKey  (ev));
  const releaseKey_ = makePassToIfNumber(ev => mMod.releaseKey(ev));

  const makeUpdateRoutine = () => {
    let oldFrameRate = mTargetFrameRate;
    const thisUpdater = () => {
      // get here after browser yield
      if (mHalt) {
        console.log('Update routine stopped.');
        return; // <- stop, no more udpates!
      }
      let start = new Date(); 
      startNewFrame(mGlContext, mShaderProgram);
      mMod.update(1 / oldFrameRate);
      if (oldFrameRate != mTargetFrameRate) {
        return setTimeout(makeUpdateRoutine(), (1000 / mTargetFrameRate));
      }
      let end = new Date();
      setTimeout(thisUpdater, Math.max(0, (1000 / oldFrameRate) - (end - start)));
    };
    return thisUpdater;
  };

  const resume = () => {
    console.log('Restarting update routine.');
    mHalt = false;
    setTimeout(makeUpdateRoutine(), (1000 / mTargetFrameRate));
  };
  const stop = () => {
    console.log('Stopping update routine.');
    mHalt = true;
  };

  return Object.freeze({
    // call once per page load
    startUp: (getGlContext, module, keymapper) => {
      setPlatformCallbacks(() =>
        mShaderProgram);
      mGlContextGetter = getGlContext;
      resetContext();
      mKeyMapper = keymapper;
      mMod       = startUpModule(module);
      mMod.startUp();
      
      resume();
    },
    setTargetFrameRate: fps => mTargetFrameRate = fps,
    targetFrameRate: () => mTargetFrameRate,
    restart: () => mMod.startUp(),
    onContextRestore: () => {
      console.log('Attempting to restore context');
      resetContext();
      resume();
    },
    onContextLost: stop,
    pressKey  : ev => pressKey_(mKeyMapper(ev)),
    releaseKey: ev => releaseKey_(mKeyMapper(ev)),
    resume,
    stop
  });
})();

})();