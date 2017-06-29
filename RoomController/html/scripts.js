
function success(status) { return status >= 200 && status < 300; }

function scale_form() {
    var siteWidth = 400;
    var scale = screen.width / siteWidth
    document.querySelector('meta[name="viewport"]').setAttribute('content', 'width=' + siteWidth + ', initial-scale=' + scale + '');
}

function get_object(obj, id, callback = (x) => { }) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'api/' + obj + (id ? '/' + id : ''), true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState != 4) return;
        if (xhr.status == 200) {
            var result = JSON.parse(xhr.responseText);
            callback(result);
        }
    }
    xhr.send();
}

function save_object(obj, rule, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open(rule.id ? 'PUT' : 'POST', 'api/' + obj + (rule.id ? '/' + rule.id : ''), true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState != 4) return;
        if (success(xhr.status)) callback();
    }
    xhr.send(JSON.stringify(rule));
}

function delete_object(obj, id, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('DELETE', 'api/' + obj + '/' + id, true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState != 4) return;
        if (success(xhr.status)) callback();
    }
    xhr.send();
}

function get_parameter(sParam) {
    var sPageURL = window.location.search.substring(1);
    var sURLVariables = sPageURL.split('&');
    for (var i = 0; i < sURLVariables.length; i++) {
        var sParameterName = sURLVariables[i].split('=');
        if (sParameterName[0] == sParam) return sParameterName[1];
    }
}
