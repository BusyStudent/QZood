// ==UserScript==
// @name         Userscript!
// @version      0.1
// @description  Inject the XMLHttpRequest
// @match        https://*/*
// @match        http://*/*
// @grant        none
// ==/UserScript==

console.error('Inject XMLHttpRequest begin');

// From
var XHR = XMLHttpRequest.prototype;

var open = XHR.open;
var send = XHR.send;
var setRequestHeader = XHR.setRequestHeader;

XHR.open = function(method, url) {
    this._method = method;
    this._url = url;
    this._requestHeaders = {};

    console.error('[UserScript::XMLHttpRequest] Open +' + method + ' ' + url)

    return open.apply(this, arguments);
};

XHR.setRequestHeader = function(header, value) {
    this._requestHeaders[header] = value;
    return setRequestHeader.apply(this, arguments);
};

XHR.send = function(postData) {

    this.addEventListener('load', function() {
        try {
            console.error(`[UserScript::XMLHttpRequest] Loaded: ${this.status} ${this.responseText}`);
        }
        catch (v) {
            console.error(`[UserScript::XMLHttpRequest] Loaded: ${this.status}`);
        }
    });
    this.addEventListener('readystatechange', function() {

    });

    return send.apply(this, arguments);
};


// From https://stackoverflow.com/questions/44728723/hook-all-fetch-api-ajax-requests
var originalFetch = fetch;
fetch = (input, init) => {
    console.error(`[UserScript::fetch] ${input}`);
    return originalFetch(input, init).then(response => {
        // it is not important to create new Promise in ctor, we can await existing one, then wrap result into new one
        return new Promise((resolve) => {
            response.clone() // we can invoke `json()` only once, but clone do the trick
                .json()
                .then(json => {
                    console.error(`[UserScript::fetch] Result ${json}`);
                    resolve(response);
                });
        });
    });
};


console.error('Inject XMLHttpRequest end');