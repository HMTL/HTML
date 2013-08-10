ko.bindingHandlers.simplePicker = {
    init: function(element, valueAccessor, allBindingsAccessor, viewModel, bindingContext) {
        // This will be called when the binding is first applied to an element
        // Set up any initial state, event handlers, etc. here
        
        var id_of = function(class_name) {
        	var el = $(element).find('.' + class_name);
        	if(!el.attr('id')) el.uniqueId();
        	return el.attr('id');
        };

        var mypicker = new SimplePicker(id_of('huewell'),id_of('slwell'),id_of('resultwell'));
	    mypicker.setRGBRangeControls(id_of("red"),id_of("green"),id_of("blue"));
	    mypicker.setHSLRangeControls(id_of("hue"),id_of("saturation"),id_of("lightness"));
	    mypicker.setHSLInput(id_of("hslval"));
	    mypicker.setRGBInput(id_of("rgbval"));
    },
    update: function(element, valueAccessor, allBindingsAccessor, viewModel, bindingContext) {
        // This will be called once when the binding is first applied to an element,
        // and again whenever the associated observable changes value.
        // Update the DOM element based on the supplied values here.
    }
};

function AppViewModel() {
	var template_picker = function(control) {
		var template_name = control['type']();
		return $('#'+template_name).length > 0 ? template_name : "not_found";
	};

	var devices = ko.observableArray(window.devices.map(function(device){return new Device(device)}));

	return {
		template_picker: template_picker,
		devices: devices()
	}
};

function Device(data) {
	var self = this;
	self.name = ko.observable(data.name);
	self.address = ko.observable(data.address);

	self.controls = ko.observableArray(data.controls.map(function(control){
		return new Control(control);
	}));
	return self;
};

function Control(options) {
	var self = this;
	var option_key;
	for(option_key in options) {
		self[option_key] = ko.observable(options[option_key]);
	}

	self.send_updated_value_to_server = function() {
		var command_data = {id: self.id(), };
		$.post('/', command_data)
	};

	return self;
};

$(document).ready(function(){ko.applyBindings(new AppViewModel());});