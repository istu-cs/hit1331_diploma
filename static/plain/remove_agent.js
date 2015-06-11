function remove_agent(agent_id)
{
    $.post("remove", {
        agent_id: agent_id
    }).done(function(data) {
        $("#agent_" + agent_id).remove();
    }).fail(function(data) {
        alert("Unable to remove agent id = " + agent_id);
    });
}
