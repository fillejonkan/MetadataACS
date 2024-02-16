/*!
 * AXIS Camera services 1.0.0
 *
 * Copyright 2014 Pandos Dipp
 *
 * Depends:
 *	jquery.js
 *
 *
 *  Functions:
 *  
 *  loadEventDeclarations( function(xmlData){ } )
 *    xmlData: "wstop:TopicSet" (xml text)
 *
 *  prettyXML( xml )
 *    formats xml to tabbed readable xml
 *
 *  xmlSubSection( xmlTextData, subNode )
 *    returns a subsection of xmlTextData
 * 
 */

function loadEventDeclarations( replyCallback )
{
  $.ajax({ type: "POST", url: "/vapix/services", data: soapEnvelopeEventDeclaration, dataType: "text", cache: false, success: onLoadEventDeclarations, userCallback: replyCallback});
}

function loadApplicationLog( applicationName, replyCallback )
{
  $.ajax({ type: "GET", url: "/axis-cgi/admin/systemlog.cgi?appname=" + applicationName, dataType: "text", cache: false, success: replyCallback });
}

function xmlSubSection( xmlData, subNode )
{
  var startTag = "<" + subNode;
  var endTag = "</" + subNode + ">";
  var startIndex = xmlData.search( startTag );
  var endIndex = xmlData.search( endTag );
  var resultLength = endIndex - startIndex + endTag.length;
  return xmlData.substr(startIndex,resultLength);
} 

function prettyLOG( text )
{
  //2014-01-01T01:02:03.420+02:00 axis-00408c000000 [ INFO    ] getmestarted[0]: starting getmestarted
  var reg1 = /\..*\[\s/mg;  //Match ".420+02:00 axis-00408c000000 [ " 
  text = text.replace(reg1, ' [ ');

  var reg2 = /\]\s.*\]:/mg;  //Match "] getmestarted[0]:"
  text = text.replace(reg2, ']');

  var reg3 = /([0-9])T([0-9])/mg; //Match "1T0"
  text = text.replace(reg3, '$1  $2');

  var reg4 = /\r\n-.*\n/g; //Match "----- Contents of SYSTEM_LOG for 'getmestarted' -----\r\n"
  text = text.replace(reg4, '');

  var reg5 = /__.*\n/g; //Match "__is_lua_application: missing package\r\n"
  text = text.replace(reg5, '');

  return text;
}

function prettyXML( xml )
{
  var formatted = '';
  var reg = /(>)(<)(\/*)/g;
  xml = xml.replace(reg, '$1\r\n$2$3');
  var pad = 0;
  $.each(xml.split('\r\n'), function(index, node) {
    var indent = 0;
    if (node.match( /.+<\/\w[^>]*>$/ )) {
        indent = 0;
    } else if (node.match( /^<\/\w/ )) {
        if (pad != 0) {
            pad -= 1;
        }
    } else if (node.match( /^<\w[^>]*[^\/]>.*$/ )) {
        indent = 1;
    } else {
        indent = 0;
    }

    var padding = '';
    for (var i = 0; i < pad; i++) {
        padding += '  ';
    }

    formatted += padding + node + '\r\n';
    pad += indent;
  });
  return formatted;
}

function onLoadEventDeclarations( data )
{
  var xmlData = xmlSubSection( data, "wstop:TopicSet" );
  if( this.userCallback )
    this.userCallback( xmlData );
}

var soapEnvelopeEventDeclaration =  '<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://www.w3.org/2003/05/soap-envelope"' + 
                                    'xmlns:SOAP-ENC="http://www.w3.org/2003/05/soap-encoding"' + 
                                    'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"' + 
                                    'xmlns:xsd="http://www.w3.org/2001/XMLSchema">' +
                                    '<SOAP-ENV:Body>' +
                                    '<m:GetEventInstances xmlns:m="http://www.axis.com/vapix/ws/event1"/>' +
                                    '</SOAP-ENV:Body>' +
                                    '</SOAP-ENV:Envelope>';
