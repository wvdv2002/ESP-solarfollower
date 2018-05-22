/* -----------------------------------------------------
  Material Design Sliders
  CodePen URL: https://codepen.io/rkchauhan/pen/xVGGpR
  By: Ravikumar Chauhan
-------------------------------------------------------- */
function rkmd_rangeSlider(selector) {
    var self, slider_width, slider_offset, curnt, sliderDiscrete, range, slider;
    self = $(selector);
    slider_width = self.width();
    slider_offset = self.offset().left;
    sliderDiscrete = self;

    sliderDiscrete.each(function(i, v) {
        curnt = $(this);
        curnt.append(sliderDiscrete_tmplt());
        range = curnt.find('input[type="range"]');
        slider = curnt.find('.slider');
        slider_fill = slider.find('.slider-fill');
        slider_handle = slider.find('.slider-handle');
        slider_label = slider.find('.slider-label');

        var range_val = parseInt(range.val());
        slider_fill.css('width', range_val + '%');
        slider_handle.css('left', range_val + '%');
        slider_label.find('span').text(range_val);
    });

    self.on('mousedown touchstart', '.slider-handle', function(e) {
        if (e.button === 2) {
            return false;
        }
        var parents = $(this).parents('.rkmd-slider');
        var slider_width = parents.width();
        var slider_offset = parents.offset().left;
        var check_range = parents.find('input[type="range"]').is(':disabled');
        if (check_range === true) {
            return false;
        }
        $(this).addClass('is-active');
        var moveFu =
            function(e) {
                var slider_new_width = e.pageX - slider_offset;

                if (slider_new_width <= slider_width && !(slider_new_width < '0')) {
                    slider_move(parents, slider_new_width, slider_width, true);
                }
            };
        var upFu =
            function(e) {
                $(this).off(handlers);
                parents.find('.is-active').removeClass('is-active');
            };

        var handlers = {
            mousemove: moveFu,
            touchmove: moveFu,
            mouseup: upFu,
            touchend: upFu
        };
        $(document).on(handlers);
    });

    self.on('mousedown touchstart', '.slider', function(e) {
        if (e.button === 2) {
            return false;
        }

        var parents = $(this).parents('.rkmd-slider');
        var slider_width = parents.width();
        var slider_offset = parents.offset().left;
        var check_range = parents.find('input[type="range"]').is(':disabled');

        if (check_range === true) {
            return false;
        }

        var slider_new_width = e.pageX - slider_offset;
        if (slider_new_width <= slider_width && !(slider_new_width < '0')) {
            slider_move(parents, slider_new_width, slider_width, true);
        }
        var upFu =
        function(e) {
            $(this).off(handlers);
        };

        var handlers = {
            mouseup: upFu,
            touchend: upFu
        };
        $(document).on(handlers);

    });
};

function sliderDiscrete_tmplt() {
    var tmplt = '<div class="slider">' +
        '<div class="slider-fill"></div>' +
        '<div class="slider-handle"><div class="slider-label"><span>0</span></div></div>' +
        '</div>';

    return tmplt;
}

function slider_move(parents, newW, sliderW, send) {
    var slider_new_val = parseInt(Math.round(newW / sliderW * 100));

    var slider_fill = parents.find('.slider-fill');
    var slider_handle = parents.find('.slider-handle');
    var range = parents.find('input[type="range"]');

    slider_fill.css('width', slider_new_val + '%');
    slider_handle.css({
        'left': slider_new_val + '%',
        'transition': 'none',
        '-webkit-transition': 'none',
        '-moz-transition': 'none'
    });

    range.val(slider_new_val);
    if (parents.find('.slider-handle span').text() != slider_new_val) {
        parents.find('.slider-handle span').text(slider_new_val);
        var number = parents.attr('id').substring(2);
        if (send) websock.send('slvalue:' + slider_new_val + ':' + number);
    }
}
