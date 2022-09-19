const driver=(()=>{"use strict";const M=`
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
`,y=`
varying highp vec2 vTextureCoord;
uniform sampler2D uSampler;
void main(void) {
  gl_FragColor = texture2D(uSampler, vTextureCoord);
}
`,v=(e,t,r)=>{const o=(c,l,d)=>{const u=c.createShader(l);return c.shaderSource(u,d),c.compileShader(u),c.getShaderParameter(u,c.COMPILE_STATUS)?u:(console.log(`Failed to compile shader: ${c.getShaderInfoLog(u)}`),c.deleteShader(u),null)},a=o(e,e.VERTEX_SHADER,t),n=o(e,e.FRAGMENT_SHADER,r),s=e.createProgram();return e.attachShader(s,a),e.attachShader(s,n),e.linkProgram(s),e.getProgramParameter(s,e.LINK_STATUS)?s:(console.log(`Unable to initialize the shader program: ${e.getProgramInfoLog(s)}`),null)},T=e=>{jsPlatform.modelMatrix.setApplierFactory(t=>{const r=e(),o=t.getUniformLocation(r,"uModelViewMatrix");return a=>t.uniformMatrix4fv(o,!1,a)}),jsPlatform.viewMatrix.setApplierFactory(t=>{const r=e(),o=t.getUniformLocation(r,"uViewMatrix");return a=>t.uniformMatrix4fv(o,!1,a)}),jsPlatform.setRenderModelAttributesNeederFactory(t=>{const r=e(),o=t.getAttribLocation(r,"aVertexPosition"),a=t.getAttribLocation(r,"aTextureCoord");return n=>n(o,a)}),jsPlatform.setTextureUnitHandlerFactory(t=>{const r=e(),o=t.getUniformLocation(r,"uSampler");return a=>t.uniform1i(o,a)}),m.setReseterFactory(t=>{const r=t.canvas.clientWidth/t.canvas.clientHeight;return()=>{const o=45*Math.PI/180,a=.1,n=100,s=mat4.create();return mat4.perspective(s,o,r,a,n),s}}),m.setApplierFactory(t=>{const r=t.getUniformLocation(e(),"uProjectionMatrix");return o=>t.uniformMatrix4fv(r,!1,o)})},F=e=>Object.freeze({startUp:e.cwrap("to_js_start_up","null"),pressKey:e.cwrap("to_js_press_key","null",["number"]),releaseKey:e.cwrap("to_js_release_key","null",["number"]),update:e.cwrap("to_js_update","null",["number"])}),m=(()=>{const e=()=>{};let t,r=()=>{throw"perspectiveMatrix.reset: reseter must be set before use."},o=()=>{throw"perspectiveMatrix.apply: applier must be set before use."},a=()=>{throw""},n=()=>{throw""};return Object.freeze({setContext:s=>{o=n(s),r=a(s)},reset:()=>e(t=r()),setReseterFactory:s=>e(a=s),apply:()=>o(t),setApplierFactory:s=>e(n=s)})})(),S=(e,t)=>{e.clearColor(.1,.2,.4,1),e.clearDepth(1),e.enable(e.DEPTH_TEST),e.depthFunc(e.LEQUAL),e.clear(e.COLOR_BUFFER_BIT|e.DEPTH_BUFFER_BIT),m.reset(),e.useProgram(t),m.apply()},h=e=>t=>{const r=e(t);typeof r=="number"&&e(r)};return(()=>{let e,t,r=()=>{throw""},o,a,n=48,s=!1;const c=()=>{a=r(),o=v(a,M,y),m.setContext(a),jsPlatform.setContext(a)},l=h(i=>e.pressKey(i)),d=h(i=>e.releaseKey(i)),u=()=>{let i=n;const p=()=>{if(s){console.log("Update routine stopped.");return}let f=new Date;if(S(a,o),e.update(1/i),i!=n)return setTimeout(u(),1e3/n);let _=new Date;setTimeout(p,Math.max(0,1e3/i-(_-f)))};return p},x=()=>{console.log("Restarting update routine."),s=!1,setTimeout(u(),1e3/n)},P=()=>{console.log("Stopping update routine."),s=!0};return Object.freeze({startUp:(i,p,f)=>{T(()=>o),r=i,c(),t=f,e=F(p),e.startUp(),x()},setTargetFrameRate:i=>n=i,targetFrameRate:()=>n,restart:()=>e.startUp(),onContextRestore:()=>{console.log("Attempting to restore context"),c(),x()},onContextLost:P,pressKey:i=>l(t(i)),releaseKey:i=>d(t(i)),resume:x,stop:P})})()})();
