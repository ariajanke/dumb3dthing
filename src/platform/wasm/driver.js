const driver = (() => {

const kBuiltinVertexShaderSource = `
attribute vec4 aVertexPosition;
attribute vec2 aTextureCoord;

uniform mat4 uModelViewMatrix;
uniform mat4 uProjectionMatrix;

varying highp vec2 vTextureCoord;
void main(void) {
  gl_Position = uProjectionMatrix * uModelViewMatrix * aVertexPosition;
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

const startUpWebGl = gl => {
  gl.clearColor(0.1, 0.6, 0.1, 1);
  gl.clear(gl.COLOR_BUFFER_BIT);

  const shaderProgram = initShaderProgram(gl, 
    kBuiltinVertexShaderSource, kBuiltinFragmentShaderSource);

  const programInfo = {
    program: shaderProgram,
    attribLocations: {
      vertexPosition: gl.getAttribLocation(shaderProgram, "aVertexPosition"),
      textureCoord: gl.getAttribLocation(shaderProgram, "aTextureCoord"),
    },
    uniformLocations: {
      projectionMatrix: gl.getUniformLocation(
        shaderProgram,
        "uProjectionMatrix"
      ),
      modelViewMatrix: gl.getUniformLocation(shaderProgram, "uModelViewMatrix"),
      uSampler: gl.getUniformLocation(shaderProgram, "uSampler"),
    }
  };
  
  jsPlatform.setContext(gl);
  
  // Browsers copy pixels from the loaded image in top-to-bottom order —
  // from the top-left corner; but WebGL wants the pixels in bottom-to-top
  // order — starting from the bottom-left corner. So in order to prevent
  // the resulting image texture from having the wrong orientation when
  // rendered, we need to make the following call, to cause the pixels to
  // be flipped into the bottom-to-top order that WebGL expects.
  // See jameshfisher.com/2020/10/22/why-is-my-webgl-texture-upside-down
  gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

  // platform has a lot of "must sets"
  // but this greatly eases things in the main cpp file
  // afterall the jsPlatform is supposed to take care of everything on the js 
  // side for C++
  jsPlatform.modelMatrix.setApplier(matrix => {
    gl.uniformMatrix4fv(
      programInfo.uniformLocations.modelViewMatrix,
      false, matrix);
  });
  jsPlatform.projectionMatrix.setReseter(() => {
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
  jsPlatform.projectionMatrix.setApplier(matrix => {
    gl.uniformMatrix4fv(
      programInfo.uniformLocations.projectionMatrix,
      false, matrix);
  });
  jsPlatform.setRenderModelAttributes(
    programInfo.attribLocations.vertexPosition,
    programInfo.attribLocations.textureCoord);
  jsPlatform.setTextureUnitHandler(unit =>
    gl.uniform1i(programInfo.uniformLocations.uSampler, unit));
};
  
const startUpModule = (module, keymapper) => {
  const pressKey_   = module.cwrap('to_js_press_key'  , 'null', ['number']);
  const releaseKey_ = module.cwrap('to_js_release_key', 'null', ['number']);
  
  return Object.freeze({
    startUp   : module.cwrap('to_js_start_up'   , 'null'),
    pressKey  : ev => (ev = keymapper(ev)) ? pressKey_  (ev) : undefined,
    releaseKey: ev => (ev = keymapper(rv)) ? releaseKey_(ev) : undefined,
    update    : module.cwrap('to_js_update', 'null', ['number'])
  });
};

const startUp = (gl, module, keymapper) => {
  startUpWebGl(gl);
  return startUpModule(module, keymapper);
};

return (() => {
  const kUninit = 'uninit';
  let mod = {
    pressKey  : () => { throw kUninit; },
    releaseKey: () => { throw kUninit; },
    update    : () => { throw kUninit; }
  };
  return Object.freeze({
    startUp: (gl, module, keymapper) => {
      mod = Object.assign({}, startUp(gl, module, keymapper))
    },
    update: et => mod.update(et),
    pressKey: ev => mod.pressKey(ev),
    releaseKey: ev => mod.releaseKey(ev)
  });
})();

})();