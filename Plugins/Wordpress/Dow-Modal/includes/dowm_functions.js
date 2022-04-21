jQuery(document).ready(function() {
    jQuery.fn.showModal = function() {
        element = jQuery(this);
        if (element.is('dialog')){
            element[0].showModal();
        }
    }
    jQuery.fn.closeModal = function() {
        element = jQuery(this);
        if (element.is('dialog')){
            element[0].close();
        }
    }
    jQuery('.dowm_trigger').click(function() {
        dialog = jQuery('.dowm_modal');
        dialog.showModal();
    });

    jQuery('.dowm_static_trigger').click(function() {
        dialog = jQuery('.dowm_modal');
        dialog.showModal();
    });

    jQuery(document.body).click(function(event) {
        target = jQuery(event.target);
        if (target.parents('.dowm_modal').length == 0 && !target.hasClass('dowm_trigger') && !target.hasClass('dowm_static_trigger')) {
            dialog = jQuery('.dowm_modal');
            dialog.closeModal();
        }
    });

    jQuery('.dowm_cancel').click(function() {
        dialog = jQuery('.dowm_modal');
        dialog.closeModal();
    });
});