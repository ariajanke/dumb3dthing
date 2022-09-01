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

const jsPlatform = (() => {

// I don't think I nor anyone else is served by that first indentation...

const isPowerOf2 = x => (x & (x - 1)) == 0;
const blockReturn = () => {};

const loadImageTexture = (gl, url) => {
  const texture = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, texture);

  // Because images have to be download over the internet
  // they might take a moment until they are ready.
  // Until then put a single pixel in the texture so we can
  // use it immediately. When the image has finished downloading
  // we'll update the texture with the contents of the image.

  // ^ how will that play with my accessors? or C++ code?
  const level = 0;
  const internalFormat = gl.RGBA;
  const width = 1;
  const height = 1;
  const border = 0;
  const srcFormat = gl.RGBA;
  const srcType = gl.UNSIGNED_BYTE;
  const pixel = new Uint8Array([0, 0, 255, 255]); // opaque blue
  gl.texImage2D(
    gl.TEXTURE_2D, level, internalFormat, width, height, border, srcFormat,
    srcType, pixel);

  const image = new Image();
  image.onload = () => {
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texImage2D(
      gl.TEXTURE_2D, level, internalFormat, srcFormat, srcType, image);

    // WebGL1 has different requirements for power of 2 images
    // vs non power of 2 images so check if the image is a
    // power of 2 in both dimensions.
    if (isPowerOf2(image.width) && isPowerOf2(image.height)) {
      // Yes, it's a power of 2. Generate mips.
      gl.generateMipmap(gl.TEXTURE_2D);
    } else {
      // No, it's not a power of 2. Turn of mips and set
      // wrapping to clamp to edge
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    }
  };
  image.src = url;

  return [image, texture];
};

const mkTexture = gl => {
  let mImage;
  let mTexture;
  let mUnit;
  let mUnitIdx;
  return Object.freeze({
    // issues of timing with loading?
    height: () => mImage.height,
    width : () => mImage.width,
    load  : url => blockReturn([mImage, mTexture] = loadImageTexture(gl, url)),
    bind: doWithUnit => {
      // not sure what to do on slots???
      // do I have to implement an LRU or something (yuck!)
      gl.activeTexture(mUnit);
      gl.bindTexture(gl.TEXTURE_2D, mTexture);
      doWithUnit(mUnitIdx);
    },
    getNativeTexture: () => mTexture,
    setUnit: n => {
      const tu = gl['TEXTURE' + n];
      if (!tu) {
        throw `Failed to set texture unit to "TEXTURE${ + n}".`;
      }
      mUnit = tu;
      mUnitIdx = n;
    },
    getUnit: () => mUnitIdx,
    destroy: () => gl.deleteTexture(mTexture)
  });
};

const mkRenderModel = gl => {
  let mElements;
  let mPositions;
  let mTexturePositions;
  let mElementsCount;

  const doBufferLoad = (enumTarget, typedArr) => {
    const buf = gl.createBuffer();
    gl.bindBuffer(enumTarget, buf);
    gl.bufferData(enumTarget, typedArr, gl.STATIC_DRAW);
    return buf;
  };
  
  const doBufferRender = (buffer, attrLoc, numOfComps) => {
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.vertexAttribPointer(attrLoc, numOfComps, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(attrLoc);
  };

  const loadFromTypedArrays = (positions, texturePosits, elements) => {
    mElements = doBufferLoad(gl.ELEMENT_ARRAY_BUFFER, elements);
    mPositions = doBufferLoad(gl.ARRAY_BUFFER, positions);
    mTexturePositions = doBufferLoad(gl.ARRAY_BUFFER, texturePosits);
    mElementsCount = elements.length;
  };

  return Object.freeze({
    load: loadFromTypedArrays,
    loadFromJsArrays: (positions, texturePosits, elements) =>
      loadFromTypedArrays(new Float32Array(positions), new Float32Array(texturePosits),
                          new Uint16Array(elements)),
    render: (positionAttrLoc, textureAttrLoc) => {
      doBufferRender(mPositions, positionAttrLoc, 3);
      doBufferRender(mTexturePositions, textureAttrLoc, 2);
      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, mElements);
      gl.drawElements(gl.TRIANGLES, mElementsCount, gl.UNSIGNED_SHORT, 0);
    },
    destroy: () => {
      gl.deleteBuffer(mElements);
      gl.deleteBuffer(mPositions);
      gl.deleteBuffer(mTexturePositions);
    }
  });
};

const mkJsPlatform = () => {
  let mGlContext;
  let mPositionsAttrLoc;
  let mTexturePositionsAttrLoc;
  let mHandleTextureUnit = () => {};

  const mkIndexMap = factory => {
    let mIndex = 0;
    let mMap = {};
    return [/* create */ () => {
      const idx = mIndex;
      mMap[idx] = factory(mGlContext);
      mIndex++;
      return idx;
    }, /* destroy */ handle => {
      mMap[handle].destroy();
      mMap[handle] = undefined;
    }, /* get */ handle => {
      // don't accidentally let client write dictionary
      const rv = mMap[handle];
      if (!rv) throw "Did you forget to initialize this resource with the create function?";
      return rv;
    }];
  };

  // need things for model matrix and look at

  const modelMatrix = (() => {
    const kYAxis = [0, 1, 0];
    // these two just for basic demo
    const kXAxis = [1, 0, 0];
    const kZAxis = [0, 0, 1];
    let mMatrix;
    let mApplier = () => {};
    return Object.freeze({
      rotateY: angle => mat4.rotate(mMatrix, mMatrix, angle, kYAxis),
      rotateX: angle => mat4.rotate(mMatrix, mMatrix, angle, kXAxis),
      rotateZ: angle => mat4.rotate(mMatrix, mMatrix, angle, kZAxis),
      translate: r => mat4.translate(mMatrix, mMatrix, r),
      reset: () => blockReturn(mMatrix = mat4.create()),
      apply: () => mApplier(mMatrix),
      // !<needs to be set before start>!
      setApplier: f => blockReturn(mApplier = f)
    });
  })();

  const projectionMatrix = (() => {
    let mMatrix;
    let mReseter = () => { throw 'projectionMatrix.reset: must be set before use.'; };
    let mApplier = () => {};
    return Object.freeze({
      reset: () => blockReturn( mMatrix = mReseter() ),
      // !<needs to be set before start>!
      setReseter: f => blockReturn(mReseter = f),
      lookAt: (eye, center, up) => mat4.lookAt(mMatrix, eye, center, up),
      apply: () => mApplier(mMatrix),
      // !<needs to be set before start>!
      setApplier: f => blockReturn(mApplier = f)
    });
  })();

  const [createTexture, destroyTexture, getTexture] =
    mkIndexMap(mkTexture);
  const [createRenderModel, destroyRenderModel, getRenderModel] =
    mkIndexMap(mkRenderModel);

  return Object.freeze({
    setContext: gl => mGlContext = gl,
    createTexture    , destroyTexture    , getTexture    ,
    createRenderModel, destroyRenderModel, getRenderModel,
    // !<needs to be set before start>!
    setRenderModelAttributes: (positionAttrLoc, textureAttrLoc) => {
      mPositionsAttrLoc = positionAttrLoc;
      mTexturePositionsAttrLoc = textureAttrLoc;
    },
    renderRenderModel: handle =>
      getRenderModel(handle).render(mPositionsAttrLoc, mTexturePositionsAttrLoc),
    // !<needs to be set before start>!
    setTextureUnitHandler: handler => blockReturn(mHandleTextureUnit = handler),
    bindTexture: handle => getTexture(handle).bind(mHandleTextureUnit),
    modelMatrix, projectionMatrix
  });
};

return mkJsPlatform();

})(); // end of jsPlatform construction