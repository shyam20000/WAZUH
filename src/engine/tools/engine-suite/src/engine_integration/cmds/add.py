import shared.resource_handler as rs
from pathlib import Path
from .generate_manifest import run as gen_manifest
import sys

DEFAULT_API_SOCK = '/var/ossec/queue/sockets/engine-api'

class CommandsManager:
    commands_list = []
    last_command = 0

    def add_command(self, command, counter_command):
        self.commands_list.append((command, counter_command))
        return len(self.commands_list) - 1

    def execute(self):
        for idx, tup in enumerate(self.commands_list):
            try:
                tup[0]()
                print(f'Executing {idx}')
            except:
                self.last_command = idx
                print('Error, undoing from: ' + str(self.last_command))
                self.undo()
                return 1
        return 0

    def undo(self):
        undo_list = self.commands_list[:(
            self.last_command+1)]
        undo_list.reverse()
        for tup in undo_list:
            tup[1]()


def run(args, resource_handler: rs.ResourceHandler):
    api_socket = args['api_sock']

    working_path = resource_handler.cwd()
    if args['integration-path']:
        working_path = args['integration-path']
        path = Path(working_path)
        if path.is_dir():
            working_path = str(path.resolve())
        else:
            print(f'Error: Directory does not exist ')
            return -1

    print(f'Adding integration from: {working_path}')

    cm = CommandsManager()

    # Catalog Functions to functions for undo / redo
    def func_to_recursive_load_catalog(api_socket: str, path_str: str, type: str, print_name: bool = False):
        def recursive_load_catalog():
            resource_handler._recursive_command_to_catalog(
                api_socket, path_str, type, 'post', print_name)
        return recursive_load_catalog

    def func_to_recursive_delete_catalog(api_socket: str, path_str: str, type: str, print_name: bool = False):
        def recursive_delete_catalog():
            resource_handler._recursive_command_to_catalog(
                api_socket, path_str, type, 'delete', print_name)
        return recursive_delete_catalog

    # KVDB Functions to functions for undo / redo
    def func_to_recursive_create_kvdbs(api_socket: str, path_str: str, print_name: bool = False):
        def recursive_create_kvdbs():
            resource_handler._base_recursive_command_on_kvdbs(
                api_socket, path_str, 'post', print_name)
        return recursive_create_kvdbs

    def func_to_recursive_delete_kvdbs(api_socket: str, path_str: str, print_name: bool = False):
        def recursive_delete_kvdbs():
            resource_handler._base_recursive_command_on_kvdbs(
                api_socket, path_str, 'delete', print_name)
        return recursive_delete_kvdbs

    # Create kvdbs
    pos = cm.add_command(func_to_recursive_create_kvdbs(api_socket, working_path, True),
                         func_to_recursive_delete_kvdbs(api_socket, working_path, True))
    print(f'Kvdbs creation,  \texecution order: {pos}')
    # Recursively add all components to the catalog
    pos = cm.add_command(func_to_recursive_load_catalog(api_socket, working_path, 'decoders', True),
                         func_to_recursive_delete_catalog(api_socket, working_path, 'decoders', True))
    print(f'Decoders creation,\texecution order: {pos}')
    pos = cm.add_command(func_to_recursive_load_catalog(api_socket, working_path, 'rules', True),
                         func_to_recursive_delete_catalog(api_socket, working_path, 'rules', True))
    print(f'Rules creation,  \texecution order: {pos}')
    pos = cm.add_command(func_to_recursive_load_catalog(api_socket, working_path, 'outputs', True),
                         func_to_recursive_delete_catalog(api_socket, working_path, 'outputs', True))
    print(f'Outputs creation,\texecution order: {pos}')
    pos = cm.add_command(func_to_recursive_load_catalog(api_socket, working_path, 'filters', True),
                         func_to_recursive_delete_catalog(api_socket, working_path, 'filters', True))
    print(f'Filters creation,\texecution order: {pos}')

    if cm.execute() == 0:
        # Creates a manifest.yml if it doesn't exists
        manifest_file = working_path + '/manifest.yml'
        path = Path(manifest_file)
        if not path.is_file():
            args = {'output-path':working_path} #Is there a better way of doing this?
            print(f'"manifest.yml" not found creating one...')
            gen_manifest(args,resource_handler)

        # integration name is taken from the directory name
        name = path.resolve().parent.name
        print(f'Loading integration [{name}] manifest...')
        try:
            manifest = resource_handler.load_file(manifest_file)
            resource_handler.add_catalog_file(
                api_socket, 'integration', f'integration/{name}/0', manifest, rs.Format.YML)
        except:
            #TODO: should be neccesary to undo the whole proccess for this single step?
            print('Couldnt add integration to the store, try manually with catalog update')
            resource_handler.delete_catalog_file(
                api_socket, 'integration', f'integration/{name}/0')
    else:
        print('Error occur on the adding proccess, policy cleaned')


def configure(subparsers):
    parser_add = subparsers.add_parser(
        'add', help='Add integration components to the Engine\' Catalog')
    parser_add.add_argument('-a', '--api-sock', type=str, default=DEFAULT_API_SOCK, dest='api_sock',
                            help=f'[default="{DEFAULT_API_SOCK}"] Engine instance API socket path')

    parser_add.add_argument('-p', '--integration-path', type=str, dest='integration-path',
                            help=f'[default=current directory] Integration directory path')

    #bool
    # parser_add.add_argument('-v', '--verbose', type=str, help=f'prints Traceback on error messages')
    # TODO: check if this is a clearer approach
    # sys.tracebacklimit = 0

    parser_add.set_defaults(func=run)
