//39.101.135.108:20010
//192.168.1.130:10010
const httpApi = "https://192.168.1.130:10011";
//const httpApi = "http://39.101.135.108:20010";


const sendOffer = async (desc) => {
  const res = await fetch(httpApi + "/x2rtc/v1/pushstream", {
    method: 'POST', // *GET, POST, PUT, DELETE, etc.
    // headers: {
    //   'Content-Type': 'application/json'
    //   // 'Content-Type': 'application/x-www-form-urlencoded',
    // },
    body: JSON.stringify({
      streamurl: "webrtc://39.101.135.108:20010/live/1233?chanId=001&token=xxx",
      sessionid: "",
      localsdp: desc
    })
  });

  return res.json();
}

const sendRecvOnlyOffer = async (desc) => {
  const res = await fetch(httpApi + "/x2rtc/v1/pullstream", {
    method: 'POST', // *GET, POST, PUT, DELETE, etc.
    // headers: {
    //   'Content-Type': 'application/json'
    //   // 'Content-Type': 'application/x-www-form-urlencoded',
    // },
    body: JSON.stringify({
      streamurl: "webrtc://39.101.135.108:20010/live/1233?chanId=001&token=xxx&scale=4",
      sessionid: "",
      localsdp: desc
    })
  });

  return res.json();
}


const startButton = document.getElementById('startButton');
const callButton = document.getElementById('callButton');
const hangupButton = document.getElementById('hangupButton');
callButton.disabled = true;
hangupButton.disabled = true;
startButton.addEventListener('click', start);
callButton.addEventListener('click', call);
hangupButton.addEventListener('click', hangup);

let startTime;
const localVideo = document.getElementById('localVideo');
const remoteVideo = document.getElementById('remoteVideo');

localVideo.addEventListener('loadedmetadata', function() {
  console.log(`Local video videoWidth: ${this.videoWidth}px,  videoHeight: ${this.videoHeight}px`);
});

remoteVideo.addEventListener('loadedmetadata', function() {
  console.log(`Remote video videoWidth: ${this.videoWidth}px,  videoHeight: ${this.videoHeight}px`);
});

remoteVideo.addEventListener('resize', () => {
  console.log(`Remote video size changed to ${remoteVideo.videoWidth}x${remoteVideo.videoHeight} - Time since pageload ${performance.now().toFixed(0)}ms`);
  // We'll use the first onsize callback as an indication that video has started
  // playing out.
  if (startTime) {
    const elapsedTime = window.performance.now() - startTime;
    console.log('Setup time: ' + elapsedTime.toFixed(3) + 'ms');
    startTime = null;
  }
});

let localStream;
let pcPush;
let pcPlay;
const offerPushOptions = {
  offerToReceiveAudio: 0,
  offerToReceiveVideo: 0,
  iceRestart:true,
  voiceActivityDetection: true
};
const offerPlayOptions = {
  offerToReceiveAudio: 1,
  offerToReceiveVideo: 1,
  iceRestart:true,
  voiceActivityDetection: false
};

function getName(pc) {
  return (pc === pcPush) ? 'pcPush' : 'pcPlay';
}

function getOtherPc(pc) {
  return (pc === pcPush) ? pcPlay : pcPush;
}

async function start() {
  console.log('Requesting local stream');
  startButton.disabled = true;
  try {
    //const stream = await navigator.mediaDevices.getUserMedia({audio: true, video: true});
    const stream = await navigator.mediaDevices.getUserMedia({audio: true, video: {
			frameRate: 30
		}
	});
    console.log('Received local stream');
    localVideo.srcObject = stream;
    localStream = stream;
    callButton.disabled = false;
  } catch (e) {
    alert(`getUserMedia() error: ${e.name}`);
  }
}

async function call() {
  callButton.disabled = true;
  hangupButton.disabled = false;
  console.log('Starting call');
  startTime = window.performance.now();
  const videoTracks = localStream.getVideoTracks();
  const audioTracks = localStream.getAudioTracks();
  if (videoTracks.length > 0) {
    console.log(`Using video device: ${videoTracks[0].label}`);
  }
  if (audioTracks.length > 0) {
    console.log(`Using audio device: ${audioTracks[0].label}`);
  }
  const configuration = {};
  console.log('RTCPeerConnection configuration:', configuration);
  // 推流客户端
  pcPush = new RTCPeerConnection(configuration);
  console.log('Created local peer connection object pcPush');
  pcPush.addEventListener('icecandidate', e => onIceCandidate(pcPush, e));
  pcPush.addEventListener('iceconnectionstatechange', e => onIceStateChange(pcPush, e));

  // 拉流客户端
  pcPlay = new RTCPeerConnection(configuration);
  console.log('Created remote peer connection object pcPlay');
  pcPlay.addEventListener('icecandidate', e => onIceCandidate(pcPlay, e));
  pcPlay.addEventListener('iceconnectionstatechange', e => onIceStateChange(pcPlay, e));
  pcPlay.addEventListener('track', gotRemoteStream);

  var simulcast = false;
  if(!simulcast)
  {
	localStream.getTracks().forEach(track => pcPush.addTrack(track, localStream));
  }
  else
  {
	for(const audioTrack of localStream.getAudioTracks()) {
      pcPush.addTransceiver(audioTrack, {direction:  'sendrecv', streams: [localStream]});
    }

    var encodings = [
        {rid: 'h', maxBitrate: 2500000, active: true, priority: "high"},
        {rid: 'm', maxBitrate: 1500000, active: true, scaleResolutionDownBy: 2.0},
        {rid: 'l', maxBitrate: 800000, active: true, scaleResolutionDownBy: 4.0}
    ];

    for(const videoTrack of localStream.getVideoTracks()) {

        pcPush.addTransceiver(videoTrack, {direction:  'sendrecv',
          sendEncodings: encodings, streams: [localStream]});

    }
  }
  
   console.log('Added local stream to pcPush');

  try {
	  //console.log('pcPlay createOffer start');
		const recvOnlyoffer = await pcPlay.createOffer(offerPlayOptions);
		await onCreateRecvOnlyOfferSuccess(recvOnlyoffer);
	
	
		console.log('pcPush createOffer start');
		const offer = await pcPush.createOffer(offerPushOptions);
		await onCreateOfferSuccess(offer);
  } catch (e) {
    onCreateSessionDescriptionError(e);
  }
}

function onCreateSessionDescriptionError(error) {
  console.log(`Failed to create session description: ${error.toString()}`);
}

async function onCreateOfferSuccess(desc) {
  console.log(`Offer from pcPush\n${desc.sdp}`);
  console.log('pcPush setLocalDescription start');
  try {
    await pcPush.setLocalDescription(desc);
    onSetLocalSuccess(pcPush);
  } catch (e) {
    onSetSessionDescriptionError();
  }
  const { errcode, remotesdp } = await sendOffer(desc);
  if (errcode === 0) {
    console.log('remotesdp', remotesdp);
    // remotesdp.type = "answer";
    onCreateAnswerSuccess(remotesdp);
  }

  // console.log('pcPlay setRemoteDescription start');
  // try {
  //   await pcPlay.setRemoteDescription(desc);
  //   onSetRemoteSuccess(pcPlay);
  // } catch (e) {
  //   onSetSessionDescriptionError();
  // }

  // console.log('pcPlay createAnswer start');
  // Since the 'remote' side has no media stream we need
  // to pass in the right constraints in order for it to
  // accept the incoming offer of audio and video.
  // try {
  //   const answer = await pcPlay.createAnswer();
  //   await onCreateAnswerSuccess(answer);
  // } catch (e) {
  //   onCreateSessionDescriptionError(e);
  // }
}

async function onCreateRecvOnlyOfferSuccess(desc) {
  console.log(`Offer from pcPlay\n${desc.sdp}`);
  console.log('pcPlay setLocalDescription start');
  try {
    await pcPlay.setLocalDescription(desc);
    onSetLocalSuccess(pcPlay);
  } catch (e) {
    onSetSessionDescriptionError();
  }
  const { errcode, remotesdp } = await sendRecvOnlyOffer(desc);
  if (errcode === 0) {
    console.log('remotesdp', remotesdp);
    // remotesdp.type = "answer";
    onCreateRecvOnlyAnswerSuccess(remotesdp);
  }

  // console.log('pcPlay setRemoteDescription start');
  // try {
  //   await pcPlay.setRemoteDescription(desc);
  //   onSetRemoteSuccess(pcPlay);
  // } catch (e) {
  //   onSetSessionDescriptionError();
  // }

  // console.log('pcPlay createAnswer start');
  // Since the 'remote' side has no media stream we need
  // to pass in the right constraints in order for it to
  // accept the incoming offer of audio and video.
  // try {
  //   const answer = await pcPlay.createAnswer();
  //   await onCreateAnswerSuccess(answer);
  // } catch (e) {
  //   onCreateSessionDescriptionError(e);
  // }
}

function onSetLocalSuccess(pc) {
  console.log(`${getName(pc)} setLocalDescription complete`);
}

function onSetRemoteSuccess(pc) {
  console.log(`${getName(pc)} setRemoteDescription complete`);
}

function onSetSessionDescriptionError(error) {
  console.log(`Failed to set session description: ${error.toString()}`);
}

function gotRemoteStream(e) {
  if (remoteVideo.srcObject !== e.streams[0]) {
    remoteVideo.srcObject = e.streams[0];
    console.log('pcPlay received remote stream');
  }
}

async function onCreateAnswerSuccess(desc) {
  console.log(`Answer from pcPlay:\n${desc.sdp}`);
  // console.log('pcPlay setLocalDescription start');
  // try {
  //   await pcPlay.setLocalDescription(desc);
  //   onSetLocalSuccess(pcPlay);
  // } catch (e) {
  //   onSetSessionDescriptionError(e);
  // }
  console.log('pcPush setRemoteDescription start');
  try {
    await pcPush.setRemoteDescription(desc);
    onSetRemoteSuccess(pcPush);
  } catch (e) {
    onSetSessionDescriptionError(e);
  }
}

async function onCreateRecvOnlyAnswerSuccess(desc) {
  console.log(`Answer from server:\n${desc.sdp}`);
  // console.log('pcPlay setLocalDescription start');
  // try {
  //   await pcPlay.setLocalDescription(desc);
  //   onSetLocalSuccess(pcPlay);
  // } catch (e) {
  //   onSetSessionDescriptionError(e);
  // }
  console.log('pcPlay setRemoteDescription start');
  try {
    await pcPlay.setRemoteDescription(desc);
    onSetRemoteSuccess(pcPlay);
  } catch (e) {
    onSetSessionDescriptionError(e);
  }
}

async function onIceCandidate(pc, event) {
  try {
    await (getOtherPc(pc).addIceCandidate(event.candidate));
    onAddIceCandidateSuccess(pc);
  } catch (e) {
    onAddIceCandidateError(pc, e);
  }
  console.log(`${getName(pc)} ICE candidate:\n${event.candidate ? event.candidate.candidate : '(null)'}`);
}

function onAddIceCandidateSuccess(pc) {
  console.log(`${getName(pc)} addIceCandidate success`);
}

function onAddIceCandidateError(pc, error) {
  console.log(`${getName(pc)} failed to add ICE Candidate: ${error.toString()}`);
}

function onIceStateChange(pc, event) {
  if (pc) {
    console.log(`${getName(pc)} ICE state: ${pc.iceConnectionState}`);
    console.log('ICE state change event: ', event);
  }
}

function hangup() {
  console.log('Ending call');
  pcPush.close();
  pcPush = null;
  
  pcPlay.close();
  pcPlay = null;
  
  hangupButton.disabled = true;
  callButton.disabled = false;
}
