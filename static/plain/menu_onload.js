function menu_onload()
{
    path = window.location.pathname;
    $("ul.menu li a[href='" + path + "']").attr('class', 'current');
    $("ul.menu li a[href!='" + path + "']").attr('class', 'other');
}
