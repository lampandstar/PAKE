/**
 * srp.js
 * @fileOverview SRP client-side implementation
 * @description One-Way optimized SRP client-side
 * @author weir007
 */

(function (module, exports) {
	
	function srp() {//BN, crypto
		/**
		 * _constructor
		 */
		if (typeof BN === 'undefined') {
			console.error('BN library needed.');
			return;
		}
		console.log('_constructor');
	}
	
	if (typeof module === 'object') {
		modulo.exports = srp;
	} else {
		exports.SRP = srp;
	}
	
	let mhex = "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8EF4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA9AFD5138FE8376435B9FC61D2FC0EB06E3";
	
	srp.modulo = BN.red(new BN(mhex, 16));
	srp.g = new BN(2).toRed(srp.modulo);
	srp.key_byts = 32;
	//srp.modulo_byts = srp.modulo.byteLength;
	srp.modulo_byts = mhex.length >> 1;
	
	/* user id */
	srp.id = '';
	
	/**
	 * tools
	 */	
	
	function assert (val, msg) {
		if (!val) throw new Error(msg || 'Assertion failed');
	}
	
	/* XHTTP request */
	let xhr = new XMLHttpRequest();
	//xhr.open('POST', './service/service.php', true);

	function send_msg(msg, step)
	{
		var resp = '';
		xhr.open('POST', './service/service.php', false);
		xhr.onload = function() {
			//console.log('xhttp onload : '+this.response);
			resp = this.response;
			console.log('response : '+resp);
		};
		xhr.onerror = function() {
			console.error('Http request error.');
		}
		
		var data = new FormData();
		data.append('id', srp.id);
		data.append('step', step || 1);
		data.append('data', msg);
		//console.log('request : '+msg);
		xhr.send(data);
		//console.log('after sending');
		return resp;
	}  
	
	const fromHexString = hexString => new Uint8Array(hexString.match(/.{1,2}/g).map(byte => parseInt(byte, 16)));

	const toHexString = bytes => bytes.reduce((str, byte) => str + byte.toString(16).padStart(2, '0'), '');


/*
	srp.sha256 = async function(message) {
		// encode as UTF-8
		const msgBuffer = new TextEncoder('utf-8').encode(message);
		// hash the message
		const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
		// convert ArrayBuffer to Array
		const hashArray = Array.from(new Uint8Array(hashBuffer));
		// convert bytes to hex string                  
		//const hashHex = hashArray.map(b => ('00' + b.toString(16)).slice(-2)).join('');
		const hashHex = hashArray.map(b => ('00' + b.toString(16)).slice(-2)).join('');
		return hashHex;
	};
*/

	/* hex https://geraintluff.github.io/sha256/ */
	srp.hash = function sha256(ascii) {
		function ROL(value, amount) {
			return (value>>>amount) | (value<<(32 - amount));
		};
		
		var mathPow = Math.pow;
		var maxWord = mathPow(2, 32);
		var lengthProperty = 'length'
		var i, j;
		var result = ''
	
		var words = [];
		var asciiBitLength = ascii[lengthProperty]*8;
		var hash = sha256.h = sha256.h || [];
		/* Round constants */
		var k = sha256.k = sha256.k || [];
		var primeCounter = k[lengthProperty];
		var isComposite = {};
		for (var candidate = 2; primeCounter < 64; candidate++) {
			if (!isComposite[candidate]) {
				for (i = 0; i < 313; i += candidate) {
					isComposite[i] = candidate;
				}
				hash[primeCounter] = (mathPow(candidate, .5)*maxWord)|0;
				k[primeCounter++] = (mathPow(candidate, 1/3)*maxWord)|0;
			}
		}
	
		ascii += '\x80';
		while (ascii[lengthProperty]%64 - 56) ascii += '\x00';
		for (i = 0; i < ascii[lengthProperty]; i++) {
			j = ascii.charCodeAt(i);
			if (j>>8) return; // ASCII check: only accept characters in range 0-255
			words[i>>2] |= j << ((3 - i)%4)*8;
		}
		words[words[lengthProperty]] = ((asciiBitLength/maxWord)|0);
		words[words[lengthProperty]] = (asciiBitLength)
		
		// process each chunk
		for (j = 0; j < words[lengthProperty];) {
			var w = words.slice(j, j += 16); // The message is expanded into 64 words as part of the iteration
			var oldHash = hash;
			// This is now the undefinedworking hash", often labelled as variables a...g
			// (we have to truncate as well, otherwise extra entries at the end accumulate
			hash = hash.slice(0, 8);
			
			for (i = 0; i < 64; i++) {
				var i2 = i + j;
				// Expand the message into 64 words
				// Used below if 
				var w15 = w[i - 15], w2 = w[i - 2];
	
				// Iterate
				var a = hash[0], e = hash[4];
				var temp1 = hash[7]
					+ (ROL(e, 6) ^ ROL(e, 11) ^ ROL(e, 25)) // S1
					+ ((e&hash[5])^((~e)&hash[6])) // ch
					+ k[i]
					// Expand the message schedule if needed
					+ (w[i] = (i < 16) ? w[i] : (
							w[i - 16]
							+ (ROL(w15, 7) ^ ROL(w15, 18) ^ (w15>>>3)) // s0
							+ w[i - 7]
							+ (ROL(w2, 17) ^ ROL(w2, 19) ^ (w2>>>10)) // s1
						)|0
					);
				// This is only used once, so *could* be moved below, but it only saves 4 bytes and makes things unreadble
				var temp2 = (ROL(a, 2) ^ ROL(a, 13) ^ ROL(a, 22)) // S0
					+ ((a&hash[1])^(a&hash[2])^(hash[1]&hash[2])); // maj
				
				hash = [(temp1 + temp2)|0].concat(hash); // We don't bother trimming off the extra ones, they're harmless as long as we're truncating when we do the slice()
				hash[4] = (hash[4] + temp1)|0;
			}
			
			for (i = 0; i < 8; i++) {
				hash[i] = (hash[i] + oldHash[i])|0;
			}
		}
		
		for (i = 0; i < 8; i++) {
			for (j = 3; j + 1; j--) {
				var b = (hash[i]>>(j*8)) & 255;
				result += ((b < 16) ? 0 : '') + b.toString(16);
			}
		}
		return result;
	};

	srp.rand = function(len) {
		var bytes = new Uint8Array(len);
		return Array.from(crypto.getRandomValues(bytes)).map(b => ('00' + b.toString(16)).slice(-2)).join('');
	};
	
	/**
	 * Global SRP variales
	 */
	var P, a, A, K, M1 = '';
	
	/**
	 * SRP client program
	 */
	srp.login_step1 = function(pass) {
		/* 1. send ID, A=g**a to server */
		a = new BN(this.rand(this.modulo_byts), 16);
		P = this.hash(pass);
		A = this.g.redPow(a).toString(16);
		return A;
	};

	srp.login_step2 = function(msg) {
		/*
		  msg = s|B|u|M2
		  var s = this.rand(32),
		      u = new BN(this.rand(32), 16),
		  	  B = this.rand(32);
		*/
		var params = msg.split('|');
		assert(params.length === 4, 'message format error.');
		
		var s = params[0], B = params[1],
		    u = new BN(params[2], 16), M2 = params[2];
		
		/* 2. x=H(s,P)*/
		var x = new BN(this.hash(s.concat(this.P)), 16);
		console.log('x : '+x.toString(16));
		console.log('u : '+u.toString(16));
		console.log('M2 : '+M2);
		/* 3. S=(B-g**x)**(a+u*x) */
		//var S = new BN(B, 16).sub(this.g.redPow(x)).redPow(a.add(u.mul(x))).toString(16);
		var S = this.g.redPow(x).redPow(a.add(u.mul(x))).toString(16);
		console.log('S : '+S);
		/* 4. K=H(S) */
		K = this.hash(S);
		console.log('K : '+K);
		/* 5. M1=H(A,B,K) */
		//console.log('M1 : '+this.hash(A.concat(B).concat(K));
		M1 = this.hash(A.concat(B).concat(K));
		return M1 === M2 ? fromHexString(K) : null;
	};
	/*
	srp.login_step3 = function(M1, M2) {
		return M1 === M2 ? fromHexString(K) : null;
	};*/


	/**
	 * Program entry-point
	 */
	srp.start = function(id, pass) {
		assert(id && pass, 'id and password are required.');
		this.id = id;
		var buf1 = srp.login_step1(pass);/* A */
		
		console.log('A = '+buf1);
		var buf2 = send_msg(buf1, 1);/* s|B|u|M2 from server */
		assert(buf2, 'SRP step 1 failed.');

		buf1 = srp.login_step2(buf2) || '';/* K or '' */
		
		//console.log('step2 returns '+buf1);
		/* send M1 */
		buf2 = send_msg(M1, 2);
		assert(buf2, 'SRP step 2 failed.');
		
		return buf1 || null;
		//return srp.login_step3(buf1, buf2);
	};
		
})(typeof module === 'undefined' || module, this);