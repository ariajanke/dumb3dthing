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
"use strict";

// I don't think I nor anyone else is served by that first indentation...

const blockReturn = () => {};

const textureLoader = (() => {
  let mGlContext;
  let mImageTextureCache = {};

  const isPowerOf2 = x => (x & (x - 1)) == 0;

  const loadImageTexture = (gl, url) => {
    if (mImageTextureCache[url]) {
      return mImageTextureCache[url];
    }
  
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
  
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
        // Generate mips.
        gl.generateMipmap(gl.TEXTURE_2D);
      } else {
        // Turn off mips and set wrapping to clamp to edge
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
      }
    };
    image.src = url;
  
    return (mImageTextureCache[url] =  { image, texture });
  };
  
  return Object.freeze({
    setContext: gl => blockReturn( mGlContext = gl ),
    loadImageTexture: url => loadImageTexture(mGlContext, url)
  });
})();

const mkTexture = gl => {
  let mImage;
  let mTexture;
  let mUnit;
  let mUnitIdx;
  return Object.freeze({
    setContext: newGl => gl = newGl,
    // issues of timing with loading?
    height: () => mImage.height,
    width : () => mImage.width,
    load  : url => {
      const { image, texture } = textureLoader.loadImageTexture(url);
      mImage = image;
      mTexture = texture;
    },
    bind: doWithUnit => {
      // not sure what to do on slots???
      // do I have to implement an LRU or something (yuck!) 
      if (!mUnit) throw 'mkTexture().bind(f): texture unit must be set.';
      gl.activeTexture(mUnit);
      gl.bindTexture(gl.TEXTURE_2D, mTexture);
      doWithUnit(gl, mUnitIdx);
    },
    getNativeTexture: () => mTexture,
    setUnit: n => {
      // gonna just assume TEXTUREN across contexts are the same (dangerously)
      const tu = gl['TEXTURE' + n];
      if (!tu) {
        throw `Failed to set texture unit to "TEXTURE${ + n}".`;
      }
      mUnit = tu;
      mUnitIdx = n;
    },
    getUnit: () => mUnitIdx,
    destroy: () => {} // gl.deleteTexture(mTexture)
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
    setContext: newGl => gl = newGl,
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
      return;
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
  let mHandleTextureUnit = () => { throw 'jsPlatform.bindTexture: must setTextureUnitHandler before use.'; };

  const mkIndexMap = factory => {
    let mIndex = 0;
    let mMap = {};
    return [/* create */ idx_ => {
      const idx = idx_ ?? mIndex++;
      mMap[idx] = factory(mGlContext);
      return idx;
    }, /* destroy */ handle => {
      mMap[handle].destroy();
      delete mMap[handle];
    }, /* get */ handle => {
      // don't accidentally let client write dictionary
      const rv = mMap[handle];
      if (!rv) throw "Did you forget to initialize this resource with the create function?";
      return rv;
    }, /* setContext */ gl =>
      Object.keys(mMap).forEach(res =>
        res.setContext(gl))
    ];
  };

  // need things for model matrix and look at
  // *all* appliers and reseters now have a "gl" parameter

  const modelMatrix = (() => {
    const kYAxis = [0, 1, 0];
    // these two just for basic demo
    const kXAxis = [1, 0, 0];
    const kZAxis = [0, 0, 1];
    let mMatrix;
    let mGlContext;
    let mApplier = () => { throw 'modelMatrix.apply: applier must be set before use.'; };
    return Object.freeze({
      setContext: gl => mGlContext = gl,
      rotateY: angle => mat4.rotate(mMatrix, mMatrix, angle, kYAxis),
      rotateX: angle => mat4.rotate(mMatrix, mMatrix, angle, kXAxis),
      rotateZ: angle => mat4.rotate(mMatrix, mMatrix, angle, kZAxis),
      translate: r => mat4.translate(mMatrix, mMatrix, r),
      reset: () => blockReturn(mMatrix = mat4.create()),
      apply: () => mApplier(mGlContext, mMatrix),
      // !<needs to be set before start>!
      setApplier: f => blockReturn(mApplier = f)
    });
  })();
  if (false) {
  // gonna deprecate/move this to the driver
  const projectionMatrix = (() => {
    let mMatrix;
    let mReseter = () => { throw 'projectionMatrix.reset: reseter must be set before use.'; };
    let mApplier = () => { throw 'projectionMatrix.apply: applier must be set before use.'; };
    return Object.freeze({
      reset: () => blockReturn( mMatrix = mReseter() ),
      // !<needs to be set before start>!
      setReseter: f => blockReturn(mReseter = f),
      // lookAt: (eye, center, up) => mat4.lookAt(mMatrix, eye, center, up),
      apply: () => mApplier(mMatrix),
      // !<needs to be set before start>!
      setApplier: f => blockReturn(mApplier = f)
    });
  })();
  }

  const viewMatrix = (() => {
    let mMatrix = mat4.create();
    let mGlContext;
    let mApplier = () => { throw 'viewMatrix.apply: applier must be set before use.'; };
    return Object.freeze({
      setContext: gl => mGlContext = gl,
      // lookAt "resets" the matrix
      lookAt: (eye, center, up) => mat4.lookAt(mMatrix, eye, center, up),
      apply: () => mApplier(mGlContext, mMatrix),
      // !<needs to be set before start>!
      setApplier: f => blockReturn(mApplier = f)
    });
  })();

  const [createTexture, destroyTexture, getTexture, 
         setContextForAllTextures] =
    mkIndexMap(mkTexture);
  const [createRenderModel, destroyRenderModel, getRenderModel,
         setContextForAllRenderModels] =
    mkIndexMap(mkRenderModel);

  const contextSetters = [
    setContextForAllRenderModels, setContextForAllTextures,
    textureLoader.setContext, viewMatrix.setContext, modelMatrix.setContext];
  return Object.freeze({
    setContext: gl => {
      mGlContext = gl;
      contextSetters.forEach(f => f(gl));
    },
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
    modelMatrix, viewMatrix
  });
};

return mkJsPlatform();

})(); // end of jsPlatform construction