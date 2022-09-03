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
const kMustHaveInvalidated = Symbol();

const textureLoader = (() => {
  let mImageTextureCache = {};
  
  const isPowerOf2 = x => (x & (x - 1)) == 0;

  const makeTextureCreator = (url, image) => 
    (gl, mustHaveInvalidatedOrUndef) =>
  {
    // call setContext to clear this cache
    if (mImageTextureCache[url].texture) {
      if (mustHaveInvalidatedOrUndef) { // <- not actually useful?!
        // ^ how would this be tested?!
        throw `Cache Safety: textureLoader's cache was no invalided`;
      }
      return mImageTextureCache[url].texture;
    }

    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
  
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
    return ( mImageTextureCache[url]['texture'] = texture );
  };

  const loadImage = (url, answerWhenReady) => {
    if (mImageTextureCache[url]) {
      answerWhenReady(makeTextureCreator(url, mImageTextureCache[url].image));
    }
  
    const image = new Image();
    image.onload = () => {
      mImageTextureCache[url] = { image };
      answerWhenReady(makeTextureCreator(url, image));
    };
    
    image.src = url;
  };
  
  return Object.freeze({
    //setContext: gl => blockReturn( mGlContext = gl ),
    invalidateTextureCache: () => { // call me when reseting context!
      Object.keys(mImageTextureCache).forEach(url => {
        // I think this is okay...
        delete mImageTextureCache[url].texture;
        mImageTextureCache[url].texture = undefined;
      });
    },
    loadImage,
  });
})();

const mkTexture = () => {
  let mGl;
  let mTextureCreator; // <- this not being set is well defined
  let mTexture;
  let mUnit;
  let mUnitIdx;

  const refreshTexture = mustHaveInvalidated => {
    mTexture = mTextureCreator(mGl, mustHaveInvalidated);
    mUnit = mGl[`TEXTURE${mUnitIdx}`];
  };

  return Object.freeze({
    setContext: newGl => {
      mGl = newGl;
      mTexture = undefined;
      if (!mTextureCreator) return;
      refreshTexture(/*kMustHaveInvalidated*/);
    },
    // issues of timing with loading?
    height: () => { throw 'idk what to do'; },
    width : () => { throw 'idk what to do'; },
    load  : url =>
      textureLoader.loadImage(url, textureCreator => {
        console.log(`Loading image for ${url} complete.`);
        mTextureCreator = textureCreator;
      }),
    bind: doWithUnit => {
      if (!mTexture) {
        if (!mTextureCreator) {
          console.log('Cannot bind texture yet! Skipping!');
          return; // <- without binding :c
        }
        console.log('Refreshing texture on bind call.')
        refreshTexture();
      }
      
      mGl.activeTexture(mUnit);
      mGl.bindTexture(mGl.TEXTURE_2D, mTexture);
      doWithUnit(mUnitIdx);
    },
    getNativeTexture: () => mTexture,
    setUnit: n => blockReturn( mUnitIdx = n ),
    getUnit: () => mUnitIdx,
    destroy: () => { if (false) gl.deleteTexture(mTexture); }
  });
};

const mkRenderModel = () => {
  let mGl;
  let mElements;
  let mPositions;
  let mTexturePositions;
  let mElementsCount;
  let mRefreshModel;// = () => { throw 'renderModel.setContext load* must be called first.'; };

  const doBufferLoad = (enumTarget, typedArr) => {
    const buf = mGl.createBuffer();
    mGl.bindBuffer(enumTarget, buf);
    mGl.bufferData(enumTarget, typedArr, mGl.STATIC_DRAW);
    return buf;
  };
  
  const doBufferRender = (buffer, attrLoc, numOfComps) => {
    mGl.bindBuffer(mGl.ARRAY_BUFFER, buffer);
    mGl.vertexAttribPointer(attrLoc, numOfComps, mGl.FLOAT, false, 0, 0);
    mGl.enableVertexAttribArray(attrLoc);
  };

  const loadFromTypedArrays = (positions, texturePosits, elements) =>
    (mRefreshModel = makeRefresherFunction(positions, texturePosits, elements))();

  const makeRefresherFunction = (positions, texturePosits, elements) => {
    mElementsCount = elements.length; // <- does not go bad
    return () => {
      mElements = doBufferLoad(mGl.ELEMENT_ARRAY_BUFFER, elements);
      mPositions = doBufferLoad(mGl.ARRAY_BUFFER, positions);
      mTexturePositions = doBufferLoad(mGl.ARRAY_BUFFER, texturePosits);
    };
  };

  return Object.freeze({
    setContext: newGl => {
      mGl = newGl;
      if (mRefreshModel) mRefreshModel();
    },
    load: loadFromTypedArrays,
    loadFromJsArrays: (positions, texturePosits, elements) =>
      loadFromTypedArrays(new Float32Array(positions), new Float32Array(texturePosits),
                          new Uint16Array(elements)),
    render: (positionAttrLoc, textureAttrLoc) => {
      doBufferRender(mPositions, positionAttrLoc, 3);
      doBufferRender(mTexturePositions, textureAttrLoc, 2);
      mGl.bindBuffer(mGl.ELEMENT_ARRAY_BUFFER, mElements);
      mGl.drawElements(mGl.TRIANGLES, mElementsCount, mGl.UNSIGNED_SHORT, 0);
    },
    destroy: () => {
      return;
      mGl.deleteBuffer(mElements);
      mGl.deleteBuffer(mPositions);
      mGl.deleteBuffer(mTexturePositions);
    }
  });
};

const mkJsPlatform = () => {
  
  let mHandleTextureUnit = () => { throw 'jsPlatform.bindTexture: must setTextureUnitHandler before use.'; };
  let mRenderModelAttributesAccepter = () => { throw 'jsPlatform.renderRenderModel: must setRenderModelAttributesNeeder before use.'; };
  let mRenderModelAttributesAccepterFactory = () => { throw ''; };
  const mkIndexMap = factory => {
    let mIndex = 0;
    let mMap = {};
    let mGl;
    return [/* create */ idx_ => {
      const idx = idx_ ?? mIndex++;
      if (mMap[idx]) {
        throw `indexMap.create: key "${idx}" already in use.`;
      }
      mMap[idx] = factory();
      mMap[idx].setContext(mGl);
      return idx;
    }, /* destroy */ handle => {
      mMap[handle].destroy();
      delete mMap[handle];
    }, /* get */ handle => {
      // don't accidentally let client write dictionary
      const rv = mMap[handle];
      if (!rv) throw "Did you forget to initialize this resource with the create function?";
      return rv;
    }, /* setContext */ gl => {
      Object.keys(mMap).forEach(key =>
        mMap[key].setContext(gl));
      mGl = gl;
    }];
  };

  // need things for model matrix and look at
  // *all* appliers and reseters now have a "gl" parameter

  const modelMatrix = (() => {
    const kYAxis = [0, 1, 0];
    // these two just for basic demo
    const kXAxis = [1, 0, 0];
    const kZAxis = [0, 0, 1];
    let mMatrix;
    //let mGlContext;
    let mApplier = () => { throw 'modelMatrix.apply: applier must be set before use.'; };
    let mApplierFactory = () => { throw 'modelMatrix.setContext: applier factor not set.'; };
    return Object.freeze({
      setContext: gl => blockReturn( mApplier = mApplierFactory(gl) ),
      rotateY: angle => mat4.rotate(mMatrix, mMatrix, angle, kYAxis),
      rotateX: angle => mat4.rotate(mMatrix, mMatrix, angle, kXAxis),
      rotateZ: angle => mat4.rotate(mMatrix, mMatrix, angle, kZAxis),
      translate: r => mat4.translate(mMatrix, mMatrix, r),
      reset: () => blockReturn(mMatrix = mat4.create()),
      apply: () => mApplier(mMatrix),
      // !<needs to be set before start>!
      setApplierFactory: f => blockReturn( mApplierFactory = f )
    });
  })();

  const viewMatrix = (() => {
    let mMatrix = mat4.create();
    //let mGlContext;
    let mApplierFactory;
    let mApplier = () => { throw 'viewMatrix.apply: applier must be set before use.'; };
    return Object.freeze({
      setContext: gl => blockReturn( mApplier = mApplierFactory(gl) ),
      // lookAt "resets" the matrix
      lookAt: (eye, center, up) => mat4.lookAt(mMatrix, eye, center, up),
      apply: () => mApplier(mMatrix),
      // !<needs to be set before start>!
      setApplierFactory: f => blockReturn(mApplierFactory = f)
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
    viewMatrix.setContext, modelMatrix.setContext];

  let mTextureUnitHandlerFactory = () => { throw ''; };
  return Object.freeze({
    // the idea is, you set this function per context restoration...
    setContext: gl => {
      // call the factory and reset the function-function that takes two attributes
      // v must come before resetting textures
      textureLoader.invalidateTextureCache();
      contextSetters.forEach(f => f(gl));
      mRenderModelAttributesAccepter = mRenderModelAttributesAccepterFactory(gl);
      mHandleTextureUnit = mTextureUnitHandlerFactory(gl);
    },
    createTexture    , destroyTexture    , getTexture    ,
    createRenderModel, destroyRenderModel, getRenderModel,
    renderRenderModel: handle =>
      mRenderModelAttributesAccepter( getRenderModel(handle).render ),
    bindTexture: handle => getTexture(handle).bind(mHandleTextureUnit),
    // !<matricies need to have factories set>!
    modelMatrix,
    viewMatrix,
    // !<needs to be set before start>!
    setTextureUnitHandlerFactory: factory => 
      blockReturn( mTextureUnitHandlerFactory = factory),
    // !<needs to be set before start>!
    // provide me with an fn, that takes another fn, which itself takes the attrs
    setRenderModelAttributesNeederFactory: factory => 
      blockReturn( mRenderModelAttributesAccepterFactory = factory )
  });
};

return mkJsPlatform();

})(); // end of jsPlatform construction